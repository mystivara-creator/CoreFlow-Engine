#include "coreflow/trial_history.hpp"
#include "coreflow/durable_file.hpp"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <sys/stat.h>
#include <utility>

namespace coreflow {
namespace {
std::uint64_t fingerprint(std::string_view text) noexcept {
    if (text.empty()) return 0;
    std::uint64_t value=14695981039346656037ULL;
    for (const char raw:text) { const auto c=static_cast<unsigned char>(raw); value^=c; value*=1099511628211ULL; }
    return value;
}
bool numeric(std::string_view text) noexcept {
    bool digit=false,point=false;
    if (!text.empty() && (text.front()=='-' || text.front()=='+')) text.remove_prefix(1);
    for (const char c:text) {
        if (c=='.' && !point) { point=true; continue; }
        if (c<'0' || c>'9') return false;
        digit=true;
    }
    return digit;
}
bool field(std::string_view value,std::size_t limit=1024U) noexcept {
    if (value.empty() || value.size()>limit) return false;
    for (const char raw:value) { const auto c=static_cast<unsigned char>(raw); if (c<32U || c==127U) return false; }
    return true;
}
}
void TrialHistory::setForeground(std::string_view identity) noexcept { foreground_=fingerprint(identity); }
TrialConditions TrialHistory::conditions(const RuntimeSample& s,RuntimeState state) const noexcept {
    TrialConditions c; c.mode=state==RuntimeState::ThermalGuard ? 1U : state==RuntimeState::Pressure ? 2U : 0U;
    c.charging_known=s.charging_telemetry_available; c.charging=s.charging; c.foreground=foreground_;
    c.frame_period_ns=s.frame_period_ns<=1000000000ULL ? s.frame_period_ns : 0U;
    const auto set=[&](std::size_t i,bool available,double value,double max) {
        if (available && std::isfinite(value) && value>=0. && value<=max) {
            c.available|=std::uint32_t{1}<<i; c.values[i]=value;
        }
    };
    set(0,s.thermal_available,static_cast<double>(std::max(s.hottest_thermal_millidegrees,s.thermal_millidegrees))/1000.,200.);
    set(1,s.cpu_utilization_available,s.cpu_utilization,1.);
    set(2,s.mem_total_kb>0U,s.mem_available_ratio,1.);
    set(3,s.psi_available,s.psi_cpu_some_avg10,1.); set(4,s.psi_available,s.psi_memory_some_avg10,1.);
    set(5,s.psi_available,s.psi_io_some_avg10,1.); set(6,s.io_latency_available,s.io_avg_latency_ms,1.e9);
    set(7,s.gpu_busy_available,s.gpu_busy_ratio,1.); set(8,s.frame_jank_available,s.frame_jank_rate,1.);
    set(9,s.load_available,s.load1,1.e6); return c;
}
bool TrialHistory::materiallyChanged(const TrialConditions& a,const TrialConditions& b,ResourceDomain domain) noexcept {
    if (a.mode!=b.mode || (a.charging_known && b.charging_known && a.charging!=b.charging) ||
        (a.foreground && b.foreground && a.foreground!=b.foreground)) return true;
    if (domain!=ResourceDomain::Memory && domain!=ResourceDomain::Io && a.frame_period_ns && b.frame_period_ns &&
        a.frame_period_ns!=b.frame_period_ns) return true;
    std::uint32_t relevant=1U;
    switch (domain) {
        case ResourceDomain::CpuFreq: case ResourceDomain::Scheduler: case ResourceDomain::UClamp: case ResourceDomain::CpuSet:
            relevant|=(1U<<1U)|(1U<<3U)|(1U<<8U)|(1U<<9U); break;
        case ResourceDomain::Memory: relevant|=(1U<<2U)|(1U<<4U); break;
        case ResourceDomain::Io: relevant|=(1U<<5U)|(1U<<6U); break;
        case ResourceDomain::Gpu: relevant|=(1U<<7U)|(1U<<8U); break;
        default: relevant=1023U; break;
    }
    constexpr std::array<double,10> delta{4.,.20,.10,.10,.10,.10,10.,.20,.05,1.};
    for (std::size_t i=0;i<delta.size();++i) {
        const auto bit=std::uint32_t{1}<<i;
        if (!(relevant&bit) || !(b.available&bit)) continue;
        if (!(a.available&bit)) {
            // Loss of evidence never unlocks a retry. New sensors unlock only
            // when they reveal a real bottleneck, not merely a fresh poll.
            if ((i==1U && b.values[i]>=.75) || ((i==3U || i==4U || i==5U) && b.values[i]>=.15) ||
                (i==6U && b.values[i]>=25.) || (i==8U && b.values[i]>=.10)) return true;
            continue;
        }
        double threshold=delta[i];
        if (i==6U || i==9U) threshold=std::max(threshold,std::abs(a.values[i])*.50);
        if (std::abs(a.values[i]-b.values[i])+1.e-9>=threshold) return true;
    }
    return false;
}
bool TrialHistory::updateQuietRelease(const TrialConditions& accepted,const TrialConditions& now,
                                     bool tuning_needed,unsigned& samples) noexcept {
    if (tuning_needed || !materiallyChanged(accepted,now,ResourceDomain::Vendor)) samples=0;
    else samples=std::min(samples,2U)+1U;
    return samples>=3U;
}
const char* TrialHistory::blockReason(std::string_view path,std::string_view current,ResourceDomain domain,
                                     const TrialConditions& now,std::string_view requested) const noexcept {
    if (!ready()) return "trial_history_checkpoint_unavailable";
    if (entries_.size()>=kMaxEntries-4U && std::none_of(entries_.begin(),entries_.end(),[&](const Entry& e) {
        return materiallyChanged(e.when,now,e.member.domain);
    })) return "trial_history_budget_waits_for_changed_conditions";
    for (auto it=entries_.rbegin();it!=entries_.rend();++it) {
        if (it->member.path!=path || it->member.domain!=domain || materiallyChanged(it->when,now,domain)) continue;
        if (it->accepted) {
            if (current==it->member.target) return "accepted_configuration_held_for_matching_conditions";
        } else if (current==it->member.previous || current==it->member.target) {
            // Distinct categorical alternatives may compete; scalar variants
            // on the same failed surface wait for new conditions instead of
            // evading history with a slightly different number.
            const bool scalar=numeric(it->member.target) &&
                !(domain==ResourceDomain::CpuFreq && path.ends_with("/scaling_governor"));
            if (scalar || (!requested.empty() && requested==it->member.target))
                return "previous_trial_deferred_until_material_conditions_change";
        }
    }
    return nullptr;
}
bool TrialHistory::remember(const std::vector<TrialMember>& members,const TrialConditions& when,
                            BaselineIntelligence::Outcome outcome,bool accepted,std::string_view group) noexcept {
    if (!healthy_ || members.empty()) return false;
    if (when.mode>2U || when.available>1023U || when.frame_period_ns>1000000000ULL || static_cast<unsigned>(outcome)>4U ||
        (accepted && outcome!=BaselineIntelligence::Outcome::Beneficial && outcome!=BaselineIntelligence::Outcome::Neutral))
        return healthy_=false;
    for (std::size_t i=0;i<when.values.size();++i) {
        const double max=i==0U ? 200. : i==6U ? 1.e9 : i==9U ? 1.e6 : 1.;
        if (!std::isfinite(when.values[i]) || when.values[i]<0. || when.values[i]>max) return healthy_=false;
    }
    try {
        auto next=entries_;
        for (const auto& member:members) {
            if (!field(member.path) || !field(member.previous) || !field(member.target) || !isTunableResourceDomain(member.domain))
                return healthy_=false;
            next.erase(std::remove_if(next.begin(),next.end(),[&](const Entry& e) {
                return e.member.path==member.path && e.member.previous==member.previous && e.member.target==member.target &&
                    !materiallyChanged(e.when,when,member.domain);
            }),next.end());
            next.push_back({member,when,accepted,static_cast<unsigned>(outcome),fingerprint(group)});
            if (next.size()>kMaxEntries) {
                const auto obsolete=std::find_if(next.begin(),next.end(),[&](const Entry& e) {
                    return materiallyChanged(e.when,when,e.member.domain);
                });
                if (obsolete==next.end()) return healthy_=false;
                next.erase(obsolete);
            }
        }
        entries_=std::move(next); dirty_=true; return true;
    } catch (...) { healthy_=false; return false; }
}
bool TrialHistory::initialize(std::string path,std::string scope) noexcept {
    try {
        path_=std::move(path); scope_=std::move(scope); entries_.clear(); dirty_=false; healthy_=false;
        if (!field(scope_,4096U)) return false;
        struct stat st{};
        if (::lstat(path_.c_str(),&st)!=0) { healthy_=errno==ENOENT; return healthy_; }
        std::string bytes;
        if (!readBoundedPrivateFile(path_,bytes) || bytes.empty() || bytes.back()!='\n') return false;
        std::istringstream in(bytes); in.imbue(std::locale::classic());
        std::string header,scope_read;
        if (!std::getline(in,header) || header!="COREFLOW_TRIAL_HISTORY_V1" || !(in>>std::quoted(scope_read)) || !field(scope_read,4096U)) return false;
        std::vector<Entry> parsed; std::string marker;
        while (in>>marker) {
            if (marker=="END") { in>>std::ws; if (!in.eof()) return false; healthy_=true;
                if (scope_read==scope_) entries_=std::move(parsed); else { dirty_=true; return flush(); }
                return true;
            }
            if (marker!="T" || parsed.size()>=kMaxEntries) return false;
            Entry e; unsigned domain=0,accepted=0,known=0,charging=0;
            if (!(in>>domain>>accepted>>e.outcome>>e.group>>e.when.mode>>known>>charging>>e.when.foreground>>e.when.frame_period_ns>>e.when.available)) return false;
            if (domain>static_cast<unsigned>(ResourceDomain::Vendor) || accepted>1U || e.outcome>4U || e.when.mode>2U || known>1U || charging>1U || e.when.available>1023U || e.when.frame_period_ns>1000000000ULL) return false;
            e.member.domain=static_cast<ResourceDomain>(domain); e.accepted=accepted!=0U;
            e.when.charging_known=known!=0U; e.when.charging=charging!=0U;
            if (!isTunableResourceDomain(e.member.domain)) return false;
            for (std::size_t i=0;i<e.when.values.size();++i) {
                double& v=e.when.values[i]; if (!(in>>v) || !std::isfinite(v) || v<0.) return false;
                const double max=i==0U ? 200. : i==6U ? 1.e9 : i==9U ? 1.e6 : 1.;
                if (v>max) return false;
            }
            if (!(in>>std::quoted(e.member.path)>>std::quoted(e.member.previous)>>std::quoted(e.member.target)) ||
                !field(e.member.path) || !field(e.member.previous) || !field(e.member.target)) return false;
            parsed.push_back(std::move(e));
        }
        return false;
    } catch (...) { healthy_=false; return false; }
}
bool TrialHistory::flush() noexcept {
    if (!healthy_) return false;
    if (!dirty_) return true;
    try {
        std::ostringstream out; out.imbue(std::locale::classic()); out<<std::setprecision(std::numeric_limits<double>::max_digits10);
        out<<"COREFLOW_TRIAL_HISTORY_V1\n"<<std::quoted(scope_)<<'\n';
        for (const auto& e:entries_) {
            out<<"T "<<static_cast<unsigned>(e.member.domain)<<' '<<e.accepted<<' '<<e.outcome<<' '<<e.group<<' '<<e.when.mode<<' '
                <<e.when.charging_known<<' '<<e.when.charging<<' '<<e.when.foreground<<' '<<e.when.frame_period_ns<<' '<<e.when.available;
            for (const double v:e.when.values) out<<' '<<v;
            out<<' '<<std::quoted(e.member.path)<<' '<<std::quoted(e.member.previous)<<' '<<std::quoted(e.member.target)<<'\n';
        }
        out<<"END\n";
        if (!writeDurablePrivateFile(path_,out.str())) return false;
        dirty_=false; return true;
    } catch (...) { return false; }
}
} // namespace coreflow
