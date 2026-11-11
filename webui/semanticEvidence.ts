export type SemanticDescriptorState = 'candidate' | 'validated';
export type SemanticFunctionalDomain = 'unresolved' | 'cpu' | 'gpu' | 'vm';
export type SemanticAssessmentState = 'unresolved' | 'candidate' | 'corroborated' | 'verified';
export type SemanticFreshness = 'fresh' | 'stale';

export interface SemanticDomainAssessment {
  state: SemanticAssessmentState;
  domain: SemanticFunctionalDomain;
  confidence: number;
  corroboratingRelations: number;
}
export interface SemanticEvidenceBinding {
  bindingState: SemanticDescriptorState;
  domain: SemanticFunctionalDomain;
  domainAssessment: SemanticDomainAssessment;
  generation: string;
  observedAtMonotonicNs: string;
  ageMs: number | null;
  freshness: SemanticFreshness;
}
export interface SemanticEvidence {
  schemaVersion: 1;
  status: 'available' | 'unavailable';
  generation: string | null;
  observedAtMonotonicNs: string | null;
  ageMs: number | null;
  freshness: SemanticFreshness;
  bindings: SemanticEvidenceBinding[];
}

const isRecord = (value: unknown): value is Record<string, unknown> =>
  typeof value === 'object' && value !== null && !Array.isArray(value);
const isFiniteNumber = (value: unknown): value is number =>
  typeof value === 'number' && Number.isFinite(value);
const isNullableFiniteNumber = (value: unknown): value is number | null =>
  value === null || isFiniteNumber(value);
const isDomain = (value: unknown): value is SemanticFunctionalDomain =>
  value === 'unresolved' || value === 'cpu' || value === 'gpu' || value === 'vm';
const isUnsignedDecimalString = (value: unknown): value is string =>
  typeof value === 'string' && /^[0-9]+$/.test(value);
const isFreshness = (value: unknown): value is SemanticFreshness =>
  value === 'fresh' || value === 'stale';
const isAssessmentState = (value: unknown): value is SemanticAssessmentState =>
  value === 'unresolved' || value === 'candidate' ||
  value === 'corroborated' || value === 'verified';

const isBinding = (value: unknown): value is SemanticEvidenceBinding => {
  if (!isRecord(value) || !isRecord(value.domainAssessment)) return false;
  const assessment = value.domainAssessment;
  return (
    (value.bindingState === 'candidate' || value.bindingState === 'validated') &&
    isDomain(value.domain) &&
    isAssessmentState(assessment.state) &&
    isDomain(assessment.domain) &&
    isFiniteNumber(assessment.confidence) &&
    assessment.confidence >= 0 &&
    assessment.confidence <= 1 &&
    Number.isInteger(assessment.corroboratingRelations) &&
    (assessment.corroboratingRelations as number) >= 0 &&
    (assessment.corroboratingRelations as number) <= 3 &&
    isUnsignedDecimalString(value.generation) &&
    isUnsignedDecimalString(value.observedAtMonotonicNs) &&
    isNullableFiniteNumber(value.ageMs) &&
    (value.ageMs === null || value.ageMs >= 0) &&
    isFreshness(value.freshness)
  );
};

/** Validate the daemon-owned schema without deriving binding state or freshness. */
export const isSemanticEvidence = (value: unknown): value is SemanticEvidence => {
  if (
    !isRecord(value) ||
    value.schemaVersion !== 1 ||
    (value.status !== 'available' && value.status !== 'unavailable') ||
    !isFreshness(value.freshness) ||
    !Array.isArray(value.bindings) ||
    !value.bindings.every(isBinding)
  ) return false;

  if (value.status === 'unavailable') {
    return value.generation === null &&
      value.observedAtMonotonicNs === null &&
      value.ageMs === null &&
      value.freshness === 'stale' &&
      value.bindings.length === 0;
  }

  return isUnsignedDecimalString(value.generation) &&
    isUnsignedDecimalString(value.observedAtMonotonicNs) &&
    isNullableFiniteNumber(value.ageMs) &&
    (value.ageMs === null || value.ageMs >= 0) &&
    value.bindings.length > 0;
};
