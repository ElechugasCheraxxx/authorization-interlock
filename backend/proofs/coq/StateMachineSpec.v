(* Interlock Formal Specification: Mission State Machine Transitions *)
Require Import String.
Require Import List.

Inductive OperationPhase :=
  | INIT
  | BRIEFING
  | RECON
  | ANALYSIS
  | VULN_RESEARCH
  | EXPLOITATION
  | POST_EXPLOITATION
  | POST_OP
  | REPORTING
  | CLEANUP
  | PAUSED
  | CLOSED.

Definition eq_op_phase (a b : OperationPhase) : bool :=
  match a, b with
  | INIT, INIT => true
  | BRIEFING, BRIEFING => true
  | RECON, RECON => true
  | ANALYSIS, ANALYSIS => true
  | VULN_RESEARCH, VULN_RESEARCH => true
  | EXPLOITATION, EXPLOITATION => true
  | POST_EXPLOITATION, POST_EXPLOITATION => true
  | POST_OP, POST_OP => true
  | REPORTING, REPORTING => true
  | CLEANUP, CLEANUP => true
  | PAUSED, PAUSED => true
  | CLOSED, CLOSED => true
  | _, _ => false
  end.

Definition IsLegalTransition (from to : OperationPhase) : bool :=
  match from, to with
  | INIT, BRIEFING => true
  | BRIEFING, RECON => true
  | RECON, ANALYSIS => true
  | ANALYSIS, VULN_RESEARCH => true
  | VULN_RESEARCH, EXPLOITATION => true
  | EXPLOITATION, POST_EXPLOITATION => true
  | POST_EXPLOITATION, POST_OP => true
  | POST_OP, REPORTING => true
  | REPORTING, CLEANUP => true
  | CLEANUP, CLOSED => true
  | _, PAUSED => true
  | PAUSED, _ => true
  | f, t => if (eq_op_phase f t) then true else false
  end.

Fixpoint IsValidPath (start : OperationPhase) (path : list OperationPhase) : bool :=
  match path with
  | nil => true
  | next :: rest => if IsLegalTransition start next then IsValidPath next rest else false
  end.

(* Extraction to OCaml for runtime validation *)
Require Extraction.
Extraction Language OCaml.

Extract Inductive bool => "bool" [ "true" "false" ].
Extract Inductive list => "list" [ "[]" "(::)" ].

Extraction "state_machine_validator.ml" IsLegalTransition IsValidPath.
