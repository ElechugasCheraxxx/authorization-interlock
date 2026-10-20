(* Interlock Formal Specification: Math & Compression Correctness *)
Require Import ZArith.
Require Import Extraction.

Definition certified_add (a b : Z) : Z := (a + b)%Z.

(* Extraction to OCaml for the FormalBridge *)
Extraction Language OCaml.
Extract Inductive Z => "int" [ "0" "" "" ] " (fun _ -> assert false) ".
Extract Inlined Constant Z.add => "(+)".
Extraction "math_validator.ml" certified_add.
