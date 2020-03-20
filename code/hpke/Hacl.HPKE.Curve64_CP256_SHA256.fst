module Hacl.HPKE.Curve64_CP256_SHA256

open Hacl.Meta.HPKE

module IDH = Hacl.Impl.Generic.DH
module IHK = Hacl.Impl.Generic.HKDF
module IHash = Hacl.Impl.Generic.Hash
module IAEAD = Hacl.Impl.Instantiate.AEAD

friend Hacl.Meta.HPKE

#set-options "--fuel 0 --ifuel 0"

let setupBaseI = hpke_setupBaseI_higher #cs True IHK.hkdf_expand256 IHK.hkdf_extract256 IHash.hash_sha256 IDH.secret_to_public_c64 IDH.dh_c64

let setupBaseR = hpke_setupBaseR_higher #cs True IHK.hkdf_expand256 IHK.hkdf_extract256 IHash.hash_sha256 IDH.dh_c64 IDH.secret_to_public_c64

let sealBase = hpke_sealBase_higher #cs True setupBaseI IAEAD.aead_encrypt_cp256

let openBase = hpke_openBase_higher #cs True setupBaseR IAEAD.aead_decrypt_cp256
