open Pdg_validator
open State_machine_validator
open Data_surgery_validator

(* Conversion helpers *)
let rec coq_nat_of_int n =
  if n <= 0 then Pdg_validator.O
  else Pdg_validator.S (coq_nat_of_int (n - 1))

let coq_ascii_of_char c =
  let n = Char.code c in
  let b i = ((n land (1 lsl i)) <> 0) in
  Pdg_validator.Ascii (b 0, b 1, b 2, b 3, b 4, b 5, b 6, b 7)

let ds_coq_ascii_of_char c =
  let n = Char.code c in
  let b i = ((n land (1 lsl i)) <> 0) in
  Data_surgery_validator.Ascii (b 0, b 1, b 2, b 3, b 4, b 5, b 6, b 7)

let rec coq_string_of_string s =
  let len = String.length s in
  let rec aux i =
    if i = len then Pdg_validator.EmptyString
    else Pdg_validator.String (coq_ascii_of_char s.[i], aux (i + 1))
  in aux 0

let rec ds_coq_string_of_string s =
  let len = String.length s in
  let rec aux i =
    if i = len then Data_surgery_validator.EmptyString
    else Data_surgery_validator.String (ds_coq_ascii_of_char s.[i], aux (i + 1))
  in aux 0

let phase_of_string = function
  | "INIT" -> INIT
  | "BRIEFING" -> BRIEFING
  | "RECON" -> RECON
  | "ANALYSIS" -> ANALYSIS
  | "VULN_RESEARCH" -> VULN_RESEARCH
  | "EXPLOITATION" -> EXPLOITATION
  | "POST_EXPLOITATION" -> POST_EXPLOITATION
  | "POST_OP" -> POST_OP
  | "REPORTING" -> REPORTING
  | "CLEANUP" -> CLEANUP
  | "PAUSED" -> PAUSED
  | "CLOSED" -> CLOSED
  | _ -> raise (Invalid_argument "Unknown phase")

let () =
  let args = Sys.argv in
  if Array.length args < 3 then
    exit 1;

  let cmd = args.(1) in
  let artifact = args.(2) in

  match cmd with
  | "is_acyclic" ->
      let ids = String.split_on_char ',' artifact in
      let primitives = List.map (fun id ->
        { pid = coq_string_of_string id;
          impact_weight = O;
          is_logic_flaw = false }
      ) ids in
      if Pdg_validator.isAcyclic_bool primitives then
        (print_endline "true"; exit 0)
      else
        (print_endline "false"; exit 1)
  | "is_legal_transition" ->
      let phases = String.split_on_char ',' artifact in
      (match phases with
      | [f; t] ->
          let from_p = phase_of_string f in
          let to_p = phase_of_string t in
          if State_machine_validator.isLegalTransition from_p to_p then
            (print_endline "true"; exit 0)
          else
            (print_endline "false"; exit 1)
      | _ -> exit 1)
  | "check_integrity" ->
      let parts = String.split_on_char ',' artifact in
      (match parts with
      | [raw; id; src; hash; corr] ->
          let art = {
            Data_surgery_validator.art_id = ds_coq_string_of_string id;
            Data_surgery_validator.source_node = ds_coq_string_of_string src;
            Data_surgery_validator.integrity_hash = ds_coq_string_of_string hash;
            Data_surgery_validator.is_corrupted = (corr = "true")
          } in
          if Data_surgery_validator.extraction_Integrity_bool (ds_coq_string_of_string raw) art then
            (print_endline "true"; exit 0)
          else
            (print_endline "false"; exit 1)
      | _ -> exit 1)
  | _ ->
      exit 1
