open EverCrypt.Error

open Test_utils

type curve25519_test =
  { name : string ; scalar: Bigstring.t ; input : Bigstring.t ; expected : Bigstring.t }

let tests = [
  {
    name = "Test 1";
    scalar = Bigstring.of_string "\xa5\x46\xe3\x6b\xf0\x52\x7c\x9d\x3b\x16\x15\x4b\x82\x46\x5e\xdd\x62\x14\x4c\x0a\xc1\xfc\x5a\x18\x50\x6a\x22\x44\xba\x44\x9a\xc4";
    input = Bigstring.of_string "\xe6\xdb\x68\x67\x58\x30\x30\xdb\x35\x94\xc1\xa4\x24\xb1\x5f\x7c\x72\x66\x24\xec\x26\xb3\x35\x3b\x10\xa9\x03\xa6\xd0\xab\x1c\x4c";
    expected = Bigstring.of_string "\xc3\xda\x55\x37\x9d\xe9\xc6\x90\x8e\x94\xea\x4d\xf2\x8d\x08\x4f\x32\xec\xcf\x03\x49\x1c\x71\xf7\x54\xb4\x07\x55\x77\xa2\x85\x52"
  };
  {
    name = "Test 2";
    scalar = Bigstring.of_string "\x4b\x66\xe9\xd4\xd1\xb4\x67\x3c\x5a\xd2\x26\x91\x95\x7d\x6a\xf5\xc1\x1b\x64\x21\xe0\xea\x01\xd4\x2c\xa4\x16\x9e\x79\x18\xba\x0d";
    input = Bigstring.of_string "\xe5\x21\x0f\x12\x78\x68\x11\xd3\xf4\xb7\x95\x9d\x05\x38\xae\x2c\x31\xdb\xe7\x10\x6f\xc0\x3c\x3e\xfc\x4c\xd5\x49\xc7\x15\xa4\x93";
    expected = Bigstring.of_string "\x95\xcb\xde\x94\x76\xe8\x90\x7d\x7a\xad\xe4\x5c\xb4\xb8\x73\xf8\x8b\x59\x5a\x68\x79\x9f\xa1\x52\xe6\xf8\xf7\x64\x7a\xac\x79\x57"
  }
]

let test (v: curve25519_test) t scalarmult ecdh =
  let print_result = print_result (t ^ " " ^ v.name) in

  let out_scalarmult = Bigstring.create 32 in
  let out_ecdh = Bigstring.create 32 in
  Bigstring.fill out_scalarmult '\x00';
  Bigstring.fill out_ecdh '\x00';

  scalarmult out_scalarmult v.scalar v.input;
  ecdh out_ecdh v.scalar v.input;
  if Bigstring.compare out_scalarmult v.expected = 0 && Bigstring.compare out_ecdh v.expected = 0 then
    print_result "Success"
  else
    print_result "Shared scret mismatch"

(* TODO: tests for secret_to_public, internals *)
let _ =
  List.iter (fun v -> test v "EverCrypt.Curve25519" EverCrypt.Curve25519.scalarmult EverCrypt.Curve25519.ecdh) tests;
  List.iter (fun v -> test v "Hacl.Curve25519_51" Hacl.Curve25519_51.scalarmult Hacl.Curve25519_51.ecdh) tests;
  List.iter (fun v -> test v "Hacl.Curve25519_64" Hacl.Curve25519_64.scalarmult Hacl.Curve25519_64.ecdh) tests;
  List.iter (fun v -> test v "Hacl.Curve25519_64_Slow" Hacl.Curve25519_64_Slow.scalarmult Hacl.Curve25519_64_Slow.ecdh) tests
