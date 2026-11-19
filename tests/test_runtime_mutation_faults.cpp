#include "coreflow/runtime_architecture.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
using namespace coreflow::runtime;
namespace {
struct FakeJournal final : IWriteAheadJournal {
    JournalStatus result{JournalStatus::Committed}; unsigned calls{}; std::size_t recorded{};
    JournalStatus append_and_sync(std::span<const MutationRequest> requests) noexcept override {
        ++calls; recorded=requests.size(); return result;
    }
};
struct FakeNodePort final : INodeMutationPort {
    NodeIoStatus write_status{NodeIoStatus::Ok}; unsigned writes{}, reads{};
    NodeIoResult write(const NodeDescriptor& node, const NodeValue& expected_current_value,
        const NodeValue& value, const FileIdentity& id) noexcept override {
        ++writes;
        if (node.observed_value != expected_current_value) {
            return {NodeIoStatus::Contended, node.observed_value, node.identity};
        }
        return {write_status,value,id};
    }
    NodeIoResult read(const NodeDescriptor& node) noexcept override {
        ++reads; return {NodeIoStatus::Ok,std::int64_t{20},node.identity};
    }
};
struct FakeVerifier final : IOutcomeVerifier {
    VerificationResult result{ActuatorHealth::Healthy,true}; unsigned calls{};
    VerificationResult verify(const NodeDescriptor&,const MutationRequest&,const NodeIoResult&,
        const OemBaselineEntry*) noexcept override { ++calls; return result; }
};
struct FakeHealth final : IActuatorHealthSink {
    std::vector<ActuatorHealth> values;
    void record(const NodeId&,ActuatorHealth h) noexcept override { values.push_back(h); }
};
struct Fixture {
    DeviceManifest manifest; OemBaseline baseline; std::vector<MutationRequest> requests;
    FakeJournal journal; FakeNodePort port; FakeVerifier verifier; FakeHealth health;
    AlgorithmConstants algorithm{0.1,0.2,16,4096};
    Fixture() {
        for (const char* id : {"a","b"}) {
            NodeDescriptor n; n.id=id; n.discovered_path=std::string("/sys/discovered/")+id;
            n.access=AccessState::Writable; n.write_probe=ProbeState::Available;
            n.value_kind=ValueKind::SignedInteger; n.identity={1U,id[0]=='a'?11U:12U};
            n.observed_value=std::int64_t{10}; manifest.nodes.push_back(n);
            requests.push_back({std::string("tx-")+id,id,n.identity,std::int64_t{10},std::int64_t{20},"test"});
            baseline.entries.push_back({id,std::int64_t{10},n.identity,{}});
        }
    }
    MutationBatchResult run(UnifiedMutationEngine& engine) {
        return engine.execute(manifest,baseline,requests,journal,port,verifier,health,algorithm);
    }
};
}
int main() {
    // 1: failed journal latches and performs no writes, including later batches.
    {
        Fixture f; UnifiedMutationEngine engine; f.journal.result=JournalStatus::JournalFault;
        auto first=f.run(engine); auto second=f.run(engine);
        assert(first.global_mutation_stop && second.global_mutation_stop && engine.mutation_blocked());
        assert(f.journal.calls==1 && f.port.writes==0 && f.port.reads==0);

        Fixture invalid; UnifiedMutationEngine invalid_engine;
        invalid.algorithm.maximum_mutations_per_batch=0U;
        const auto invalid_result=invalid.run(invalid_engine);
        assert(invalid_result.policy_rejected && !invalid_result.global_mutation_stop);
        assert(invalid.journal.calls==0 && invalid.port.writes==0);
    }
    // 2: missing manifest node is skipped; sibling proceeds.
    {
        Fixture f; UnifiedMutationEngine engine; f.manifest.nodes.erase(f.manifest.nodes.begin());
        auto r=f.run(engine); assert(!r.global_mutation_stop && f.journal.recorded==1);
        assert(f.port.writes==1 && r.nodes.front().health==ActuatorHealth::SkippedUnavailable);
    }
    // 3: denied write capability is isolated.
    {
        Fixture f; UnifiedMutationEngine engine; f.manifest.nodes.front().write_probe=ProbeState::PermissionDenied;
        auto r=f.run(engine); assert(!r.global_mutation_stop && f.journal.recorded==1);
        assert(f.port.writes==1 && r.nodes.front().health==ActuatorHealth::PermissionDenied);

        Fixture identity; UnifiedMutationEngine identity_engine;
        identity.manifest.nodes.front().identity={};
        identity.requests.front().expected_identity={};
        const auto identity_result=identity.run(identity_engine);
        assert(identity_result.nodes.front().health==ActuatorHealth::IdentityLost);
        assert(identity.port.writes==1);

        Fixture stale; UnifiedMutationEngine stale_engine;
        stale.manifest.nodes.front().observed_value=std::int64_t{99};
        const auto stale_result=stale.run(stale_engine);
        assert(stale_result.nodes.front().health==ActuatorHealth::OemContention);
        assert(stale.journal.recorded==1 && stale.port.writes==1);
    }
    // 4: runtime node disappearance does not suppress independent node.
    {
        Fixture f; UnifiedMutationEngine engine; f.port.write_status=NodeIoStatus::Missing;
        auto r=f.run(engine); assert(!r.global_mutation_stop && f.port.writes==2);
        assert(f.port.reads==0 && r.nodes.back().health==ActuatorHealth::SkippedUnavailable);
    }
    // 5: OEM contention is local and does not stop other actuators.
    {
        Fixture f; UnifiedMutationEngine engine; f.port.write_status=NodeIoStatus::Contended;
        auto r=f.run(engine); assert(!r.global_mutation_stop && f.port.writes==2);
        assert(r.nodes.front().health==ActuatorHealth::OemContention);
    }
    // 6: value mismatch after readback is surfaced per actuator.
    {
        Fixture f; UnifiedMutationEngine engine; f.verifier.result={ActuatorHealth::ValueMismatch,false};
        auto r=f.run(engine); assert(!r.global_mutation_stop && f.verifier.calls==2);
        assert(r.nodes.front().health==ActuatorHealth::ValueMismatch);
    }
    std::cout<<"Runtime mutation fault-injection tests passed\n";
}
