// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <string> // std::string

std::string crypto_Base64Encode(std::string strplain);
std::string crypto_RSAEncryptString(std::string pemPublicKey, std::string message);
std::string crypto_RSADecryptString(std::string pemPrivateKey, std::string ciphertext);
