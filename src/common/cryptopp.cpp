// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "cryptopp.hpp"

#include "utf8.hpp"
#include "assistant.hpp"

#include "../../3rdparty/cryptopp/base64.h"
#include "../../3rdparty/cryptopp/osrng.h"
#include "../../3rdparty/cryptopp/rsa.h"

using namespace CryptoPP;

//************************************
// Method:      GlobalRNG
// Description: 全局通用的随机数池子
// Returns:     RandomPool&
// Author:      Sola丶小克(CairoLee)  2019/07/21 19:32
//************************************
RandomPool& GlobalRNG() {
	static RandomPool randomPool;
	return randomPool;
}

//************************************
// Method:      crypto_Base64Encode
// Description: 用于将一个字符串进行 Base64 编码 (内部已经进行 UTF8 转换)
// Parameter:   std::string strplain
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2019/07/21 16:33
//************************************
std::string crypto_Base64Encode(std::string strplain) {
	std::string plainUtf8 = PandasUtf8::ansiToUtf8(strplain);

	Base64Encoder encoder(nullptr, false);
	AlgorithmParameters params = MakeParameters(Name::Pad(), true)(Name::InsertLineBreaks(), false);
	encoder.IsolatedInitialize(params);
	encoder.Put((const byte*)plainUtf8.c_str(), plainUtf8.length());
	encoder.MessageEnd();

	std::string encoded;
	size_t size = (size_t)encoder.MaxRetrievable();
	if (size) {
		encoded.resize(size);
		encoder.Get((byte*)& encoded[0], encoded.size());
		return encoded;
	}
	return ""; 
}

//************************************
// Method:      crypto_PemToOneline
// Description: 简单的将多行的公钥或私钥处理成一行
// Parameter:   std::string pemKey
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2019/07/21 23:06
//************************************
std::string crypto_PemToOneline(std::string pemKey) {
	strReplace(pemKey, "-----BEGIN PUBLIC KEY-----", "");
	strReplace(pemKey, "-----END PUBLIC KEY-----", "");
	strReplace(pemKey, "-----BEGIN PRIVATE KEY-----", "");
	strReplace(pemKey, "-----END PRIVATE KEY-----", "");
	strReplace(pemKey, "\r\n", "");
	strReplace(pemKey, "\n", "");
	strReplace(pemKey, " ", "");
	return pemKey;
}

//************************************
// Method:      crypto_RSAEncryptString
// Description: 使用公钥对信息进行加密, 信息在加密之前会被转换成 UTF8
// Parameter:   std::string pemPublicKey
// Parameter:   std::string message
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2019/07/21 19:57
//************************************
std::string crypto_RSAEncryptString(std::string pemPublicKey, std::string message) {
	if (!pemPublicKey.length()) return "";

	std::string plainUtf8 = PandasUtf8::ansiToUtf8(message);
	pemPublicKey = crypto_PemToOneline(pemPublicKey);
	StringSource strSource(pemPublicKey, true, new Base64Decoder);
	RSA::PublicKey publicKey;
	publicKey.Load(strSource);

	RSAES_OAEP_SHA_Encryptor pub(publicKey);

	std::string ciphertext;
	StringSource(plainUtf8, true, new PK_EncryptorFilter(
		GlobalRNG(), pub, new Base64Encoder(new StringSink(ciphertext), false)
	));
	return ciphertext;
}

//************************************
// Method:      crypto_RSADecryptString
// Description: 使用私钥对信息进行解密, 信息在解密之后会被转换成 GBK 等 ANSI 编码的字符串
// Parameter:   std::string pemPrivateKey
// Parameter:   std::string ciphertext
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2019/07/21 19:57
//************************************
std::string crypto_RSADecryptString(std::string pemPrivateKey, std::string ciphertext) {
	if (!pemPrivateKey.length()) return "";

	pemPrivateKey = crypto_PemToOneline(pemPrivateKey);
	StringSource strSource(pemPrivateKey, true, new Base64Decoder);
	RSA::PrivateKey privateKey;
	privateKey.Load(strSource);

	RSAES_OAEP_SHA_Decryptor priv(privateKey);

	std::string message;
	StringSource(ciphertext, true, new Base64Decoder(
		new PK_DecryptorFilter(GlobalRNG(), priv, new StringSink(message))
	));
	message = PandasUtf8::utf8ToAnsi(message);
	return message;
}
