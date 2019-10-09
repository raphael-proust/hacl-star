module Hacl.Curve25519_64

friend Hacl.Meta.Curve25519
open Hacl.Meta.Curve25519

// The Vale core.
module C = Hacl.Impl.Curve25519.Field64.Vale

#set-options "--max_fuel 0 --max_ifuel 0 --z3rlimit 100"
let point_add_and_double =
  addanddouble_point_add_and_double_higher #M64 C.fmul C.fsqr2 C.fmul1 C.fmul2 C.fsub C.fadd
let point_double =
  addanddouble_point_double_higher #M64 C.fmul2 C.fmul1 C.fsqr2 C.fsub C.fadd
let montgomery_ladder =
  generic_montgomery_ladder_higher #M64 point_double C.cswap2 point_add_and_double
let fsquare_times = finv_fsquare_times_higher #M64 C.fsqr
let finv = finv_finv_higher #M64 fsquare_times C.fmul
let store_felem = fields_store_felem_higher #M64 C.add1
let encode_point = generic_encode_point_higher #M64 store_felem C.fmul finv
let scalarmult = generic_scalarmult_higher #M64 encode_point montgomery_ladder decode_point
let secret_to_public = generic_secret_to_public_higher #M64 scalarmult
let ecdh = generic_ecdh_higher #M64 scalarmult

