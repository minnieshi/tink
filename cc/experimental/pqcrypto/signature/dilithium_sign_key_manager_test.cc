// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
///////////////////////////////////////////////////////////////////////////////

#include "tink/experimental/pqcrypto/signature/dilithium_sign_key_manager.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "tink/experimental/pqcrypto/signature/subtle/dilithium_avx2_sign.h"
#include "tink/experimental/pqcrypto/signature/subtle/dilithium_avx2_verify.h"
#include "tink/experimental/pqcrypto/signature/subtle/dilithium_key.h"
#include "tink/public_key_verify.h"
#include "tink/util/secret_data.h"
#include "tink/util/status.h"
#include "tink/util/statusor.h"
#include "tink/util/test_matchers.h"

extern "C" {
#include "third_party/pqclean/crypto_sign/dilithium2/avx2/api.h"
#include "third_party/pqclean/crypto_sign/dilithium3/avx2/api.h"
#include "third_party/pqclean/crypto_sign/dilithium5/avx2/api.h"
}

namespace crypto {
namespace tink {
namespace {

using ::crypto::tink::subtle::DilithiumPublicKeyPqclean;
using ::crypto::tink::test::IsOk;
using ::crypto::tink::util::StatusOr;
using ::google::crypto::tink::DilithiumKeyFormat;
using ::google::crypto::tink::DilithiumPrivateKey;
using ::google::crypto::tink::DilithiumPublicKey;
using ::google::crypto::tink::KeyData;
using ::testing::Eq;
using ::testing::Not;
using ::testing::SizeIs;

struct DilithiumTestCase {
  std::string test_name;
  int private_key_size = 0;
  int public_key_size = 0;
};

using DilithiumSignKeyManagerTest = testing::TestWithParam<DilithiumTestCase>;

TEST(DilithiumSignKeyManagerTest, Basic) {
  EXPECT_THAT(DilithiumSignKeyManager().get_version(), Eq(0));
  EXPECT_THAT(DilithiumSignKeyManager().key_material_type(),
              Eq(KeyData::ASYMMETRIC_PRIVATE));
  EXPECT_THAT(DilithiumSignKeyManager().get_key_type(),
              Eq("type.googleapis.com/google.crypto.tink.DilithiumPrivateKey"));
}

TEST(DilithiumSignKeyManagerTest, ValidateKeyFormat) {
  EXPECT_THAT(DilithiumSignKeyManager().ValidateKeyFormat(DilithiumKeyFormat()),
              IsOk());
}

TEST_P(DilithiumSignKeyManagerTest, PrivateKeyWrongVersion) {
  const DilithiumTestCase& test_case = GetParam();

  DilithiumKeyFormat key_format;
  key_format.set_key_size(test_case.private_key_size);

  StatusOr<DilithiumPrivateKey> private_key =
      DilithiumSignKeyManager().CreateKey(key_format);
  ASSERT_THAT(private_key.status(), IsOk());

  private_key->set_version(1);
  EXPECT_THAT(DilithiumSignKeyManager().ValidateKey(*private_key), Not(IsOk()));
}

TEST_P(DilithiumSignKeyManagerTest, CreateKey) {
  const DilithiumTestCase& test_case = GetParam();

  DilithiumKeyFormat key_format;
  key_format.set_key_size(test_case.private_key_size);

  StatusOr<DilithiumPrivateKey> private_key =
      DilithiumSignKeyManager().CreateKey(key_format);
  ASSERT_THAT(private_key.status(), IsOk());

  EXPECT_THAT(private_key->version(), Eq(0));
  EXPECT_THAT(private_key->public_key().version(), Eq(private_key->version()));
  EXPECT_THAT(private_key->key_value(), SizeIs(test_case.private_key_size));
  EXPECT_THAT(private_key->public_key().key_value(),
              SizeIs(test_case.public_key_size));
}

TEST_P(DilithiumSignKeyManagerTest, CreateKeyValid) {
  const DilithiumTestCase& test_case = GetParam();

  DilithiumKeyFormat key_format;
  key_format.set_key_size(test_case.private_key_size);

  StatusOr<DilithiumPrivateKey> private_key =
      DilithiumSignKeyManager().CreateKey(key_format);
  ASSERT_THAT(private_key.status(), IsOk());
  EXPECT_THAT(DilithiumSignKeyManager().ValidateKey(*private_key), IsOk());
}

TEST_P(DilithiumSignKeyManagerTest, CreateKeyAlwaysNew) {
  const DilithiumTestCase& test_case = GetParam();

  DilithiumKeyFormat key_format;
  key_format.set_key_size(test_case.private_key_size);

  absl::flat_hash_set<std::string> keys;
  int num_tests = 100;
  for (int i = 0; i < num_tests; ++i) {
    StatusOr<DilithiumPrivateKey> private_key =
        DilithiumSignKeyManager().CreateKey(key_format);
    ASSERT_THAT(private_key.status(), IsOk());
    keys.insert(private_key->key_value());
  }
  EXPECT_THAT(keys, SizeIs(num_tests));
}

TEST_P(DilithiumSignKeyManagerTest, GetPublicKey) {
  const DilithiumTestCase& test_case = GetParam();

  DilithiumKeyFormat key_format;
  key_format.set_key_size(test_case.private_key_size);

  StatusOr<DilithiumPrivateKey> private_key =
      DilithiumSignKeyManager().CreateKey(key_format);
  ASSERT_THAT(private_key.status(), IsOk());

  StatusOr<DilithiumPublicKey> public_key_or =
      DilithiumSignKeyManager().GetPublicKey(*private_key);
  ASSERT_THAT(public_key_or.status(), IsOk());

  EXPECT_THAT(public_key_or->version(),
              Eq(private_key->public_key().version()));
  EXPECT_THAT(public_key_or->key_value(),
              Eq(private_key->public_key().key_value()));
}

TEST_P(DilithiumSignKeyManagerTest, Create) {
  const DilithiumTestCase& test_case = GetParam();

  DilithiumKeyFormat key_format;
  key_format.set_key_size(test_case.private_key_size);

  util::StatusOr<DilithiumPrivateKey> private_key =
      DilithiumSignKeyManager().CreateKey(key_format);
  ASSERT_THAT(private_key.status(), IsOk());

  util::StatusOr<std::unique_ptr<PublicKeySign>> signer =
      DilithiumSignKeyManager().GetPrimitive<PublicKeySign>(*private_key);
  ASSERT_THAT(signer.status(), IsOk());

  util::StatusOr<DilithiumPublicKeyPqclean> dilithium_public_key =
      DilithiumPublicKeyPqclean::NewPublicKey(
          private_key->public_key().key_value(),
          subtle::DilithiumSeedExpansion::SEED_EXPANSION_SHAKE);

  util::StatusOr<std::unique_ptr<PublicKeyVerify>> verifier =
      subtle::DilithiumAvx2Verify::New(*dilithium_public_key);
  ASSERT_THAT(verifier.status(), IsOk());

  std::string message = "Some message";
  util::StatusOr<std::string> signature = (*signer)->Sign(message);
  ASSERT_THAT(signature.status(), IsOk());
  EXPECT_THAT((*verifier)->Verify(*signature, message), IsOk());
}

TEST_P(DilithiumSignKeyManagerTest, CreateDifferentKey) {
  const DilithiumTestCase& test_case = GetParam();

  DilithiumKeyFormat key_format;
  key_format.set_key_size(test_case.private_key_size);

  util::StatusOr<DilithiumPrivateKey> private_key =
      DilithiumSignKeyManager().CreateKey(key_format);
  ASSERT_THAT(private_key.status(), IsOk());

  util::StatusOr<std::unique_ptr<PublicKeySign>> signer =
      DilithiumSignKeyManager().GetPrimitive<PublicKeySign>(*private_key);
  ASSERT_THAT(signer.status(), IsOk());

  std::string bad_public_key_data(test_case.public_key_size, '@');
  util::StatusOr<DilithiumPublicKeyPqclean> dilithium_public_key =
      DilithiumPublicKeyPqclean::NewPublicKey(
          bad_public_key_data,
          subtle::DilithiumSeedExpansion::SEED_EXPANSION_SHAKE);
  util::StatusOr<std::unique_ptr<PublicKeyVerify>> verifier =
      subtle::DilithiumAvx2Verify::New(*dilithium_public_key);
  ASSERT_THAT(verifier.status(), IsOk());

  std::string message = "Some message";
  util::StatusOr<std::string> signature = (*signer)->Sign(message);
  ASSERT_THAT(signature.status(), IsOk());
  EXPECT_THAT((*verifier)->Verify(*signature, message), Not(IsOk()));
}

INSTANTIATE_TEST_SUITE_P(
    DilithiumSignKeyManagerTests, DilithiumSignKeyManagerTest,
    testing::ValuesIn<DilithiumTestCase>({
        {"Dilithium2", PQCLEAN_DILITHIUM2_AVX2_CRYPTO_SECRETKEYBYTES,
         PQCLEAN_DILITHIUM2_AVX2_CRYPTO_PUBLICKEYBYTES},
        {"Dilithium3", PQCLEAN_DILITHIUM3_AVX2_CRYPTO_SECRETKEYBYTES,
         PQCLEAN_DILITHIUM3_AVX2_CRYPTO_PUBLICKEYBYTES},
        {"Dilithium5", PQCLEAN_DILITHIUM5_AVX2_CRYPTO_SECRETKEYBYTES,
         PQCLEAN_DILITHIUM5_AVX2_CRYPTO_PUBLICKEYBYTES},
    }),
    [](const testing::TestParamInfo<DilithiumSignKeyManagerTest::ParamType>&
           info) { return info.param.test_name; });

}  // namespace

}  // namespace tink
}  // namespace crypto