# Security Policy

## Scope of use

Authorization Interlock is a platform for **authorized** security
assessment. Operating it requires:

- Signed services agreement with the target organization
- Documented Statement of Work with explicit scope
- Written authorization from the system owner (CISO / security
  operations lead)
- Engagement registered in EvidenceVault before any action is taken

**Unauthorized use against systems you do not own or have written
permission to assess is illegal in most jurisdictions.**

## Reporting a vulnerability

Found a vulnerability in this platform? Report it privately:

- Email: jhoshuastevens556@icloud.com
- Do NOT open a public issue for security reports

## Scope enforcement

The engagement lifecycle is defined as a formal state machine (see
`backend/proofs/`). It is mathematically impossible for the system to
reach an active-testing phase without completing the prior authorization
phases. A bug in the C++ implementation cannot silence a violation of the
formal invariants: both code paths are cross-checked at every phase
transition and the process aborts on divergence.
