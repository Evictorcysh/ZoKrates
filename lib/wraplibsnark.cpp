/**
 * @file wraplibsnark.cpp
 * @author Dennis Kuhnert <dennis.kuhnert@campus.tu-berlin.de>
 * @author Jacob Eberhardt <jacob.eberhardt@tu-berlin.de
 * @date 2017
 */

#include "wraplibsnark.hpp"
#include <iostream>
#include <cassert>
#include <iomanip>

// contains definition of alt_bn128 ec public parameters
#include "libsnark/algebra/curves/alt_bn128/alt_bn128_pp.hpp"

// contains required interfaces and types (keypair, proof, generator, prover, verifier)
#include "libsnark/zk_proof_systems/ppzksnark/r1cs_ppzksnark/r1cs_ppzksnark.hpp"

// contains usage example for these interfaces
//#include "libsnark/zk_proof_systems/ppzksnark/r1cs_ppzksnark/examples/run_r1cs_ppzksnark.hpp"

// How to "zkSNARK from R1CS:"
// libsnark/relations/constraint_satisfaction_problems/r1cs/examples/r1cs_examples.tcc

// How to generate R1CS Example
// libsnark/zk_proof_systems/ppzksnark/r1cs_ppzksnark/examples/run_r1cs_ppzksnark.tcc

// Interfaces for R1CS
// libsnark/relations/constraint_satisfaction_problems/r1cs/r1cs.hpp

using namespace std;
using namespace libsnark;

// conversion byte[32] <-> libsnark bigint.
libsnark::bigint<libsnark::alt_bn128_r_limbs> libsnarkBigintFromBytes(const uint8_t* _x)
{
  libsnark::bigint<libsnark::alt_bn128_r_limbs> x;

  for (unsigned i = 0; i < 4; i++) {
    for (unsigned j = 0; j < 8; j++) {
      x.data[3 - i] |= uint64_t(_x[i * 8 + j]) << (8 * (7-j));
    }
  }
  return x;
}

// conversion libsnark bigint <-> byte[32]
// assumes #limbs = 4 and size(limb) = 64 bit
std::string DecStringFromLibsnarkBigint(libsnark::bigint<libsnark::alt_bn128_r_limbs> _x)
{
  uint8_t x[32];
  for (unsigned i = 0; i < 4; i++)
                  for (unsigned j = 0; j < 8; j++)
                                  x[i * 8 + j] = uint8_t(uint64_t(_x.data[3 - i]) >> (8 * (7 - j)));

  std::stringstream ss;
  ss << std::setfill('0');
  for (unsigned i = 0; i<32; i++) {
                  ss << std::dec << std::setw(2) << (int)x[i];
  }

  return ss.str();
}

std::string HexStringFromLibsnarkBigint(libsnark::bigint<libsnark::alt_bn128_r_limbs> _x){
								uint8_t x[32];
								for (unsigned i = 0; i < 4; i++)
																for (unsigned j = 0; j < 8; j++)
																								x[i * 8 + j] = uint8_t(uint64_t(_x.data[3 - i]) >> (8 * (7 - j)));

								std::stringstream ss;
								ss << std::setfill('0');
								for (unsigned i = 0; i<32; i++) {
																ss << std::hex << std::setw(2) << (int)x[i];
								}

								return ss.str();
}

std::string outputPointG1AffineAsHex(libsnark::alt_bn128_G1 _p)
{
								libsnark::alt_bn128_G1 aff = _p;
								aff.to_affine_coordinates();
								return
																"0x" +
																HexStringFromLibsnarkBigint(aff.X.as_bigint()) +
																", 0x" +
																HexStringFromLibsnarkBigint(aff.Y.as_bigint()) +
																"";
}


std::string outputPointG1AffineAsDec(libsnark::alt_bn128_G1 _p)
{
								libsnark::alt_bn128_G1 aff = _p;
								aff.to_affine_coordinates();
                return
																"" +
																DecStringFromLibsnarkBigint(aff.X.as_bigint()) +
																", " +
																DecStringFromLibsnarkBigint(aff.Y.as_bigint()) +
																"";
}

std::string outputPointG2AffineAsHex(libsnark::alt_bn128_G2 _p)
{
								libsnark::alt_bn128_G2 aff = _p;
								aff.to_affine_coordinates();
								return
																"[0x" +
																HexStringFromLibsnarkBigint(aff.X.c1.as_bigint()) + ", 0x" +
																HexStringFromLibsnarkBigint(aff.X.c0.as_bigint()) + "], [0x" +
																HexStringFromLibsnarkBigint(aff.Y.c1.as_bigint()) + ", 0x" +
																HexStringFromLibsnarkBigint(aff.Y.c0.as_bigint()) + "]";
}

std::string outputPointG2AffineAsDec(libsnark::alt_bn128_G2 _p)
{
								libsnark::alt_bn128_G2 aff = _p;
								aff.to_affine_coordinates();
								return
																"[" +
																DecStringFromLibsnarkBigint(aff.X.c1.as_bigint()) + ", " +
																DecStringFromLibsnarkBigint(aff.X.c0.as_bigint()) + "], [" +
																DecStringFromLibsnarkBigint(aff.Y.c1.as_bigint()) + ", " +
																DecStringFromLibsnarkBigint(aff.Y.c0.as_bigint()) + "]";
}

//takes input and puts it into constraint system
r1cs_ppzksnark_constraint_system<alt_bn128_pp> createConstraintSystem(const uint8_t* A, const uint8_t* B, const uint8_t* C, const uint8_t* witness, int constraints, int variables, int inputs)
{
  r1cs_constraint_system<Fr<alt_bn128_pp> > cs;
  cs.primary_input_size = inputs;
  cs.auxiliary_input_size = variables - inputs - 1; // ~one not included

  cout << "num variables: " << variables <<endl;
  cout << "num constraints: " << constraints <<endl;
  cout << "num inputs: " << inputs <<endl;

  for (int row = 0; row < constraints; row++) {
    linear_combination<Fr<alt_bn128_pp> > lin_comb_A, lin_comb_B, lin_comb_C;

    for (int idx=0; idx<variables; idx++) {
      libsnark::bigint<libsnark::alt_bn128_r_limbs> value = libsnarkBigintFromBytes(A+row*variables*32 + idx*32);
      // cout << "C entry " << idx << " in row " << row << ": " << value << endl;
      if (!value.is_zero()) {
        cout << "A(" << idx << ", " << value << ")" << endl;
        lin_comb_A.add_term(idx, value);
      }
    }
    for (int idx=0; idx<variables; idx++) {
      libsnark::bigint<libsnark::alt_bn128_r_limbs> value = libsnarkBigintFromBytes(B+row*variables*32 + idx*32);
      // cout << "B entry " << idx << " in row " << row << ": " << value << endl;
      if (!value.is_zero()) {
        cout << "B(" << idx << ", " << value << ")" << endl;
        lin_comb_B.add_term(idx, value);
      }
    }
    for (int idx=0; idx<variables; idx++) {
      libsnark::bigint<libsnark::alt_bn128_r_limbs> value = libsnarkBigintFromBytes(C+row*variables*32 + idx*32);
      // cout << "C entry " << idx << " in row " << row << ": " << value << endl;
      if (!value.is_zero()) {
        cout << "C(" << idx << ", " << value << ")" << endl;
        lin_comb_C.add_term(idx, value);
      }
    }
    cs.add_constraint(r1cs_constraint<Fr<alt_bn128_pp> >(lin_comb_A, lin_comb_B, lin_comb_C));
  }
  for (int idx=0; idx<variables; idx++) {
    cout << "witness entry " << idx << ": " << libsnarkBigintFromBytes(witness + idx*32) << endl;
  }

  return cs;
}

// keypair generateKeypair(constraints)
r1cs_ppzksnark_keypair<alt_bn128_pp> generateKeypair(const r1cs_ppzksnark_constraint_system<alt_bn128_pp> &cs){
  // from r1cs_ppzksnark.hpp
  return r1cs_ppzksnark_generator<alt_bn128_pp>(cs);
}

// compliant with solidty verification example
void exportVerificationKey(r1cs_ppzksnark_keypair<alt_bn128_pp> keypair){
								unsigned icLength = keypair.vk.encoded_IC_query.rest.indices.size() + 1;

								cout << "\tVerification key in Solidity compliant format:{" << endl;
								cout << "\t\tvk.A = Pairing.G2Point(" << outputPointG2AffineAsDec(keypair.vk.alphaA_g2) << ");" << endl;
								cout << "\t\tvk.B = Pairing.G1Point(" << outputPointG1AffineAsDec(keypair.vk.alphaB_g1) << ");" << endl;
								cout << "\t\tvk.C = Pairing.G2Point(" << outputPointG2AffineAsDec(keypair.vk.alphaC_g2) << ");" << endl;
								cout << "\t\tvk.gamma = Pairing.G2Point(" << outputPointG2AffineAsDec(keypair.vk.gamma_g2) << ");" << endl;
								cout << "\t\tvk.gammaBeta1 = Pairing.G1Point(" << outputPointG1AffineAsDec(keypair.vk.gamma_beta_g1) << ");" << endl;
								cout << "\t\tvk.gammaBeta2 = Pairing.G2Point(" << outputPointG2AffineAsDec(keypair.vk.gamma_beta_g2) << ");" << endl;
								cout << "\t\tvk.Z = Pairing.G2Point(" << outputPointG2AffineAsDec(keypair.vk.rC_Z_g2) << ");" << endl;
								cout << "\t\tvk.IC = new Pairing.G1Point[](" << icLength << ");" << endl;
								cout << "\t\tvk.IC[0] = Pairing.G1Point(" << outputPointG1AffineAsDec(keypair.vk.encoded_IC_query.first) << ");" << endl;
								for (size_t i = 1; i < icLength; ++i)
								{
																auto vkICi = outputPointG1AffineAsDec(keypair.vk.encoded_IC_query.rest.values[i - 1]);
																cout << "\t\tvk.IC[" << i << "] = Pairing.G1Point(" << vkICi << ");" << endl;
								}
								cout << "\t\t}" << endl;

}

// compliant with solidty verification example
void exportInput(r1cs_primary_input<Fr<alt_bn128_pp>> input){
								cout << "\tInput in Solidity compliant format:{" << endl;
								for (size_t i = 0; i < input.size(); ++i)
								{
																cout << "\t\tinput[" << i << "] = " << DecStringFromLibsnarkBigint(input[i].as_bigint()) << ";" << endl;
								}
								cout << "\t\t}" << endl;
}

// compliant with solidty verification example
void exportProof(r1cs_ppzksnark_proof<alt_bn128_pp> proof){
                cout << "\tstruct Proof {\n"
                   "\t\tPairing.G1Point A;\n"
                   "\t\tPairing.G1Point A_p;\n"
                   "\t\tPairing.G2Point B;\n"
                   "\t\tPairing.G1Point B_p;\n"
                   "\t\tPairing.G1Point C;\n"
                   "\t\tPairing.G1Point C_p;\n"
                   "\t\tPairing.G1Point K;\n"
                   "\t\tPairing.G1Point H;\n"
                 "\t}" << endl;

                cout << "\t//Proof in Solidity compliant format:{" << endl;
                cout << "\t\tproof.A = Pairing.G1Point(" << outputPointG1AffineAsDec(proof.g_A.g) << ");" << endl;
                cout << "\t\tproof.A_p = Pairing.G1Point(" << outputPointG1AffineAsDec(proof.g_A.h) << ");" << endl;
                cout << "\t\tproof.B = Pairing.G2Point(" << outputPointG2AffineAsDec(proof.g_B.g) << ");" << endl;
                cout << "\t\tproof.B_p = Pairing.G1Point(" << outputPointG1AffineAsDec(proof.g_B.h) << ");" << endl;
                cout << "\t\tproof.C = Pairing.G1Point(" << outputPointG1AffineAsDec(proof.g_C.g) << ");" << endl;
                cout << "\t\tproof.C_p = Pairing.G1Point(" << outputPointG1AffineAsDec(proof.g_C.h) << ");" << endl;
                cout << "\t\tproof.H = Pairing.G1Point(" << outputPointG1AffineAsDec(proof.g_H) << ");" << endl;
                cout << "\t\tproof.K = Pairing.G1Point(" << outputPointG1AffineAsDec(proof.g_K) << ");" << endl;
                cout << "\t}" << endl;

}


bool _run_libsnark(const uint8_t* A, const uint8_t* B, const uint8_t* C, const uint8_t* witness, int constraints, int variables, int inputs)
{
  // Setup:
  // create constraint system
  r1cs_constraint_system<Fr<alt_bn128_pp>> cs;
  cs = createConstraintSystem(A, B ,C , witness, constraints, variables, inputs);

  // assign variables based on witness values, excludes ~one
  // TODO: fix adding to variable assignment!
  r1cs_variable_assignment<Fr<alt_bn128_pp> > full_variable_assignment;
  for (int i = 1; i < variables; i++) {
    // for debugging
    cout << "witness ["<< i << "]: " << DecStringFromLibsnarkBigint(libsnarkBigintFromBytes(witness + i*32)) << endl;
    cout << "fieldElement ["<< i << "]: " << DecStringFromLibsnarkBigint((Fr<alt_bn128_pp>(libsnarkBigintFromBytes(witness + i*32))).as_bigint()) << endl;

    full_variable_assignment.push_back(Fr<alt_bn128_pp>(libsnarkBigintFromBytes(witness + i*32)));
  }

  // split up variables into primary and auxiliary inputs. Does *NOT* include the constant 1 */
  // Output variables belong to primary input, helper variables are auxiliary input.
  r1cs_primary_input<Fr<alt_bn128_pp> > primary_input(full_variable_assignment.begin(), full_variable_assignment.begin() + inputs);
  r1cs_primary_input<Fr<alt_bn128_pp> > auxiliary_input(full_variable_assignment.begin() + inputs, full_variable_assignment.end());

  // for debugging
  cout << "full variable assignment :"<< endl << full_variable_assignment;

  // sanity checks
  assert(cs.num_variables() == full_variable_assignment.size());
  assert(cs.num_variables() >= inputs);
  assert(cs.num_inputs() == inputs);
  assert(cs.num_constraints() == constraints);
  assert(cs.is_satisfied(primary_input, auxiliary_input));

  //initialize curve parameters
  alt_bn128_pp::init_public_params();

  // create keypair
  r1cs_ppzksnark_keypair<alt_bn128_pp> keypair = r1cs_ppzksnark_generator<alt_bn128_pp>(cs);

	// Print VerificationKey in Solidity compatible format
	exportVerificationKey(keypair);

  // print primary input
  exportInput(primary_input);

  // Proof Generation
  r1cs_ppzksnark_proof<alt_bn128_pp> proof = r1cs_ppzksnark_prover<alt_bn128_pp>(keypair.pk, primary_input, auxiliary_input);

  // print proof
  exportProof(proof);

  // Verification
  bool result = r1cs_ppzksnark_verifier_strong_IC<alt_bn128_pp>(keypair.vk, primary_input, proof);

  return result;
}
