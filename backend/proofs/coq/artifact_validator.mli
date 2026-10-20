
val negb : bool -> bool

type sumbool =
| Left
| Right

val bool_dec : bool -> bool -> sumbool

type ascii =
| Ascii of bool * bool * bool * bool * bool * bool * bool * bool

val ascii_dec : ascii -> ascii -> sumbool

type string =
| EmptyString
| String of ascii * string

val string_dec : string -> string -> sumbool

type artifact = { art_id : string; source_node : string;
                  integrity_hash : string; is_corrupted : bool }

val extraction_Integrity_bool : string -> artifact -> bool
