#include "stdafx.h"

#if 0
//Context 初始化
int CPAcquireContextImpl() {
	int rv;

	//加载配置文件
	if ((rv = initCSP())<0) {
		LogEntry("CPAcquireContext", "initCSP error", rv, 0);
		return rv;
	}
	//加密机状态
	if ((rv = testSjl22()) != 0) {
		LogEntry("CPAcquireContext", "testSjl22 error", rv, 0);
		return rv;
	}
	return 0;
}

int CPGetProvParamImpl(HCRYPTPROV hProv, DWORD dwParam, DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen) {
	int ret = 0;
	HKEY hKey;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	//获取注册表属性
	ret = GMN_RegOpen(&hKey);
	if (ERROR_SUCCESS != ret) {
		VarLogEntry(" GMN_RegOpen", "error:", ret, 0);
		return ret;
	}

	//获取属性
	ret = GMN_RegQueryValueEx(hKey, (CHAR*)dwParam, 0, &dwFlags, pbData, pdwDataLen);
	if (ERROR_SUCCESS != ret) {
		VarLogEntry(" GMN_RegQueryValueEx", "error: ret", ret, 0);
		VarLogEntry(" GMN_RegQueryValueEx", "key: %s", -1, 0,
			(CHAR*)dwParam);
		return ret;
	}
	GMN_RegCloseKey(hKey);
	return ret;
}

int CPSetProvParamImpl(HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData, DWORD dwFlags) {
	int ret = 0;
	HKEY hKey;
	
	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	//获取注册表属性
	ret = GMN_RegOpen(&hKey);
	if (ERROR_SUCCESS != ret) {
		VarLogEntry(" GMN_RegOpen", "error", ret, 0);
		return ret;
	}
	//设置注册表属性
	ret = GMN_RegSetValueEx(hKey, (LPCSTR)dwParam, 0,
		(NULL == dwFlags ? REG_SZ : dwFlags),
		pbData, strlen((char*)pbData));
	if (ERROR_SUCCESS != ret) {
		VarLogEntry(" GMN_RegSetValueEx", "error", ret, 0);
		return ret;
	}
	GMN_RegCloseKey(hKey);
	return ret;
}

int CPDeriveKeyImpl(HCRYPTPROV hProv, ALG_ID Algid,HCRYPTHASH hBaseData,HCRYPTKEY *phKey) {
	int ret = 0;
	///////////////////////////
	int timeout = 0;
	int comid;
	CHAR cKey[256];
	UCHAR deriveKey[256];
	UCHAR checkValue[256];
	HKEY_Z *hKey_z = (HKEY_Z*)phKey;
	char *key = (CHAR*)hKey_z->key;
	int algo = Algid;
	char *data = (char*)hBaseData;
	int dataLen = strlen(data);
	int keyLen = strlen(key) / 2;
	int derivationmode = 0;
	int encmode = 0;
	char deriveKeyType[] = ZMK_TYPE;
	char derivationKeyType[] = ZMK_TYPE;
	char *iv = NULL;
	int deriveKeyLen;
	HKEY_Z *hKey_deri;
	
	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	comid = InitHsmDevice(getHsmIP(), getHsmPORT(), timeout);
	if (comid<0) {
		VarLogEntry(" InitHsmDevice", "connect error", comid, 0);
		return comid;
	}
	PackBCD(key, (unsigned char*)cKey, strlen(key));
	__try {
		ret = derivatekey(comid, 0, NULL, algo, derivationmode, encmode, deriveKeyType, derivationKeyType, keyLen, cKey, dataLen, iv, data, 0, NULL, NULL, (char*)deriveKey, (char*)checkValue);
		if (ret<0) {
			VarLogEntry("derivatekey", "error", ret, 0);
			return ret;
		}
		deriveKeyLen = strlen((CHAR*)deriveKey);
	
		hKey_deri = (HKEY_Z *)malloc(sizeof(HKEY_Z));
		if (NULL == hKey_deri) {
			VarLogEntry("CPDeriveKey", "memory error", -1, 0);
			return -1;
		}

		hKey_deri->len = deriveKeyLen;
		memcpy(hKey_deri->key, deriveKey, deriveKeyLen);
		memcpy(hKey_deri->key, checkValue, strlen((CHAR*)checkValue));
		sprintf((CHAR*)hKey_deri->ALGID,"%02d",algo);
		sprintf((CHAR*)hKey_deri->KEYLEN, "%04d", deriveKeyLen);
		*phKey = (LONG)hKey_deri;
	}
	__finally
	{
		CloseHsmDevice(comid);
	}
	//////////////////////////
	return ret;
}

int CPExportKeyImpl(HCRYPTPROV hProv, HCRYPTKEY hKey,HCRYPTKEY hPubKey,BYTE *pbData,DWORD *pdwDataLen) {
	int ret = 0;
	
	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	ret = exportrsadeskeyImpl(hKey, hPubKey, pbData, (int *)pdwDataLen);
	if (ret<0 || pdwDataLen<0) {
		VarLogEntry("CPExportKey", "exportrsadeskeyImpl error", ret, 0);
		return ret;
	}
	return ret;
}

int CPGenKeyImpl(HCRYPTPROV hProv, ALG_ID Algid,DWORD dwFlags,HCRYPTKEY *phKey) {
	int ret = 0;
	int timeout = 0;
	int comid;
	PHKEY_Z hKey = NULL;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	__try {
		comid = InitHsmDevice(getHsmIP(), getHsmPORT(), timeout);
		if (comid<0) {
			VarLogEntry(" InitHsmDevice", "connect error", comid, 0);
			return comid;
		}

		switch (Algid)
		{
		case ALGO_DESTDES:
			ret = generateKeyImpl(comid, hKey);
			if (ret<0) {
				return ret;
			}
			break;
		case SIG_ALGO_RSA:
			ret = genrsakeyImpl(hProv,dwFlags, hKey, comid);
			if (ret < 0) {
				return ret;
			}
			break;
		default:
			VarLogEntry(" CPGenKey", "Algid error", Algid, 0);
			return -1;
		}
		*phKey = (LONG)hKey;
	}
	__finally {
		CloseHsmDevice(comid);
	}
	return ret;
}

int CPGenRandomImpl(HCRYPTPROV hProv, DWORD dwLen,BYTE *pbBuffer) {
	int comid;
	int ret = 0;
	int timeout = 0;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	////////////////////////////
	comid = InitHsmDevice(getHsmIP(), getHsmPORT(), timeout);
	if (comid<0) {
		VarLogEntry(" InitHsmDevice", "connect error", comid, 0);
		return comid;
	}

	__try {
		ret = genrandom(comid, 0, NULL, dwLen, pbBuffer);
		if (ret<0) {
			VarLogEntry(" genrandom", "error", ret, 0);
			return ret;
		}
	}
	__finally {
		CloseHsmDevice(comid);
	}
	///////////////////////////
	return ret;
}

int CPGetKeyParamImpl(HCRYPTPROV hProv, HCRYPTKEY hKey,DWORD dwParam,LPBYTE pbData,LPDWORD pcbDataLen) {
	int ret = 0;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	/////
	if (hKey == NULL) {
		VarLogEntry(" CPGetKeyParam", "hKey == NULL error", -1, 0);
		return -1;
	}

	HKEY_Z * tmpKey = (HKEY_Z *)hKey;
	/*
	KP_ALGID 表示返回密钥的算法标识
	KP_BLOCKLEN表示返回密钥的算法数据块长度
	KP_KEYLEN表示返回密钥的长度
	KP_SALT 表示返回密钥的盐值
	KP_PERMISSIONS 表示返回密钥的访问权限
	KP_IV表示返回算法的初始向量
	KP_PADDING 表示返回算法的填充方式
	KP_MODE 表示返回算法的加密模式
	KP_MODE_BITS表示返回算法的加密模式的反馈位数
	KP_EFFECTIVE_KEYLEN 表示返回密钥的有效长度
	*/
	/////////////////////////////////////////////
	switch (dwParam)
	{
	case KP_ALGID:
		*pcbDataLen = strlen((CHAR*)tmpKey->ALGID);
		if (pcbDataLen == 0) {
			VarLogEntry(" CPGetKeyParam", "dwParam Empty error", dwParam, 0);
			ret = -1;
			break;
		}
		if (NULL != pbData) {
			memcpy(pbData, tmpKey->ALGID, *pcbDataLen);
		}
		ret = 0;
		break;
	case KP_BLOCKLEN:
		*pcbDataLen = strlen((CHAR*)tmpKey->BLOCKLEN);
		if (pcbDataLen == 0) {
			VarLogEntry(" CPGetKeyParam", "dwParam Empty error", dwParam, 0);
			ret = -1;
			break;
		}
		if (NULL != pbData) {
			memcpy(pbData, tmpKey->BLOCKLEN, *pcbDataLen);
		}
		ret = 0;
		break;
	case KP_KEYLEN:
		*pcbDataLen = strlen((CHAR*)tmpKey->KEYLEN);
		if (pcbDataLen == 0) {
			VarLogEntry(" CPGetKeyParam", "dwParam Empty error", dwParam, 0);
			ret = -1;
			break;
		}
		if (NULL != pbData) {
			memcpy(pbData, tmpKey->KEYLEN, *pcbDataLen);
		}
		ret = 0;
		break;
	case KP_SALT:
		*pcbDataLen = strlen((CHAR*)tmpKey->SALT);
		if (pcbDataLen == 0) {
			VarLogEntry(" CPGetKeyParam", "dwParam Empty error", dwParam, 0);
			ret = -1;
			break;
		}
		if (NULL != pbData) {
			memcpy(pbData, tmpKey->SALT, *pcbDataLen);
		}
		ret = 0;
		break;
	case KP_PERMISSIONS:
		*pcbDataLen = strlen((CHAR*)tmpKey->PERMISSIONS);
		if (pcbDataLen == 0) {
			VarLogEntry(" CPGetKeyParam", "dwParam Empty error", dwParam, 0);
			ret = -1;
			break;
		}
		if (NULL != pbData) {
			memcpy(pbData, tmpKey->PERMISSIONS, *pcbDataLen);
		}
		ret = 0;
		break;
	case KP_IV:
		*pcbDataLen = strlen((CHAR*)tmpKey->IV);
		if (pcbDataLen == 0) {
			VarLogEntry(" CPGetKeyParam", "dwParam Empty error", dwParam, 0);
			ret = -1;
			break;
		}
		if (NULL != pbData) {
			memcpy(pbData, tmpKey->IV, *pcbDataLen);
		}
		ret = 0;
		break;
	case KP_PADDING:
		*pcbDataLen = strlen((CHAR*)tmpKey->PADDING);
		if (pcbDataLen == 0) {
			VarLogEntry(" CPGetKeyParam", "dwParam Empty error", dwParam, 0);
			ret = -1;
			break;
		}
		if (NULL != pbData) {
			memcpy(pbData, tmpKey->PADDING, *pcbDataLen);
		}
		ret = 0;
		break;
	case KP_MODE:
		*pcbDataLen = strlen((CHAR*)tmpKey->MODE);
		if (pcbDataLen == 0) {
			VarLogEntry(" CPGetKeyParam", "dwParam Empty error", dwParam, 0);
			ret = -1;
			break;
		}
		if (NULL != pbData) {
			memcpy(pbData, tmpKey->MODE, *pcbDataLen);
		}
		ret = 0;
		break;
	case KP_MODE_BITS:
		*pcbDataLen = strlen((CHAR*)tmpKey->MODE_BITS);
		if (pcbDataLen == 0) {
			VarLogEntry(" CPGetKeyParam", "dwParam Empty error", dwParam, 0);
			ret = -1;
			break;
		}
		if (NULL != pbData) {
			memcpy(pbData, tmpKey->MODE_BITS, *pcbDataLen);
		}
		ret = 0;
		break;
	case KP_EFFECTIVE_KEYLEN:
		*pcbDataLen = strlen((CHAR*)tmpKey->EFFECTIVE_KEYLEN);
		if (pcbDataLen == 0) {
			VarLogEntry(" CPGetKeyParam", "dwParam Empty error", dwParam, 0);
			ret = -1;
			break;
		}
		if (NULL != pbData) {
			memcpy(pbData, tmpKey->EFFECTIVE_KEYLEN, *pcbDataLen);
		}
		ret = 0;
		break;
	default:
		ret = -1;
		VarLogEntry(" CPGetKeyParam", "dwParam error", dwParam, 0);
		break;
	}
	return ret;
}

int CPGetUserKeyImpl(HCRYPTPROV hProv, DWORD dwKeySpec,HCRYPTKEY *phUserKey) {
	int ret = 0;
	HKEY_Z * hzKey;

	DWORD dwKeyLen = sizeof(HKEY_Z);
	hzKey = (PHKEY_Z)malloc(dwKeyLen);
	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	if (NULL == hzKey) {
		LogEntry("CPGetUserKey", "Memory error", -1, 0);
		return -1;
	}
	/*
	从密钥容器中获取已持久化的用户密钥句柄:从容器中获取RSA的密钥文件ID标识和。
	dwKeySpec phUserKey 根据密钥属性获取密钥句柄
	1. AT_KEYEXCHANGE 交换密钥
	2. AT_SIGNATURE 签名密钥
	*/
	switch (dwKeySpec)
	{
	case AT_KEYEXCHANGE:
		ret = CPGetProvParamImpl(hProv,AT_KEYEXCHANGE, REG_BINARY,(BYTE*)hzKey,&dwKeyLen);
		if (ERROR_SUCCESS != ret || dwKeyLen <= 0) {
			free(hzKey);
			return -1;
			LogEntry("CPGetUserKey", "AT_KEYEXCHANGE error", ret, 0);
		}
		break;
	case AT_SIGNATURE:
		ret = CPGetProvParamImpl(hProv,AT_SIGNATURE, REG_BINARY, (BYTE *)hzKey, &dwKeyLen);
		if (ERROR_SUCCESS != ret || dwKeyLen <= 0) {
			free(hzKey);
			return -1;
			LogEntry("CPGetUserKey", "AT_SIGNATURE error", ret, 0);
		}
		break;
	default:
		LogEntry("CPGetUserKey", "dwKeySpec error", dwKeySpec, 0);
		return -1;
		break;
	}
	phUserKey = (HCRYPTKEY *)hzKey;

	return ret;
}

int CPImportKeyImpl(HCRYPTPROV hProv, const BYTE *pbData,DWORD dwDataLen,HCRYPTKEY hPubKey,DWORD dwFlags,HCRYPTKEY *phKey) {
	int ret = 0;
	HPKEY_Z * pKey;
	HKEY_Z * hKey;
	UCHAR wkLmk[255];
	int keylen;
	UCHAR cv[64];
	
	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	pKey = (HPKEY_Z*)hPubKey;
	ret = importrsadeskeyImpl((UCHAR *)pbData, dwDataLen, pKey->pvKey, pKey->pvLen, wkLmk, &keylen, cv);
	if (ret != 0) {
		VarLogEntry(" importrsadeskeyImplvoid", "error", ret, 0);
		return ret;
	}
	hKey = (HKEY_Z *)malloc(sizeof(HKEY_Z));
	if (NULL == hKey) {
		VarLogEntry(" CPImportKey", "memory error", -1, 0);
		return -1;
	}
	hKey->len = keylen;
	memcpy(hKey->key, wkLmk, keylen);
	memcpy(hKey->cv, cv, strlen((CHAR*)cv));
	*phKey = (LONG)hKey;
	return ret;
}

int CPSetKeyParamImpl(HCRYPTPROV hProv, HCRYPTKEY hKey,DWORD dwParam,BYTE *pbData,DWORD dwFlags) {
	int ret = 0;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	PHKEY_Z phKey = (PHKEY_Z)hKey;
	if (phKey == NULL) {
		VarLogEntry(" CPSetKeyParamImpl", "Memory error", -1, 0);
		return -1;
	}

	switch (dwParam)
	{
	case KP_ALGID:
		memcpy(phKey->ALGID, pbData, sizeof(pbData));
		break;
	case KP_BLOCKLEN:
		memcpy(phKey->BLOCKLEN, pbData, sizeof(pbData));
		break;
	case KP_KEYLEN:
		memcpy(phKey->KEYLEN, pbData, sizeof(pbData));
		break;
	case KP_SALT:
		memcpy(phKey->SALT, pbData, sizeof(pbData));
		break;
	case KP_PERMISSIONS:
		memcpy(phKey->PERMISSIONS, pbData, sizeof(pbData));
		break;
	case KP_IV:
		memcpy(phKey->IV, pbData, sizeof(pbData));
		break;
	case KP_PADDING:
		memcpy(phKey->PADDING, pbData, sizeof(pbData));
		break;
	case KP_MODE:
		memcpy(phKey->MODE, pbData, sizeof(pbData));
		break;
	case KP_MODE_BITS:
		memcpy(phKey->MODE_BITS, pbData, sizeof(pbData));
		break;
	case KP_EFFECTIVE_KEYLEN:
		memcpy(phKey->EFFECTIVE_KEYLEN, pbData, sizeof(pbData));
		break;
	default:
		VarLogEntry(" CPSetKeyParamImpl", "dwParam error", dwParam, 0);
		return -1;
	}
	return ret;
}

int CPDecryptImpl(HCRYPTPROV hProv, HCRYPTKEY hKey,HCRYPTHASH hHash,BOOL Final,DWORD dwFlags,BYTE *pbData,DWORD *pdwDataLen) {
	int ret = 0;
	int timeout = 0;
	int comid;
	char *key;
	int dataLen;
	char *data;
	PHKEY_Z phKey = NULL;
	char *ip; 
	int port; 
	

	ip = getHsmIP();
	port = getHsmPORT();
	key = (CHAR*)phKey->key;
	dataLen = *pdwDataLen;
	data = (CHAR*)pbData;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	phKey = (PHKEY_Z)hKey;
	if (NULL == phKey) {
		VarLogEntry(" CPDecryptImpl", "hKey error", -1, 0);
		return -1;
	}
	comid = InitHsmDevice(ip, port, timeout);
	if (comid < 0) {
		VarLogEntry(" CPDecryptImpl", "InitHsmDevice error", comid, 0);
		return comid;
	}
	/////////////////////////////////////
	__try {
		int algo = 0;
		int dataBlockFlag = 0;
		int encryptFlag = 0;
		int algoOperationMode = 0;
		int inputFormat = 1;
		int outputFormat = 1;
		char keyType[] = ZEK_TYPE;
		int paddingMode = 0;
		char paddingChar[] = "0000";
		int paddingFlag = 0;
		char *iv = NULL;
		int outFlag;

		encryptFlag = 1;
		ret = encryptDecrypt(comid, 0, NULL, algo,
			dataBlockFlag,
			encryptFlag,
			algoOperationMode,
			inputFormat,
			outputFormat,
			keyType,
			key,
			paddingMode,
			paddingChar,
			paddingFlag,
			iv,
			&outFlag,
			&dataLen,
			data);
		if (ret < 0) {
			VarLogEntry(" CPDecryptImpl", "encryptDecrypt error", ret, 0);
			return ret;
		}
	}
	__finally
	{
		CloseHsmDevice(comid);
	}
	return 0;


	return ret;
}

int CPEncryptImpl(HCRYPTPROV hProv, HCRYPTKEY hKey,HCRYPTHASH hHash,BOOL Final,DWORD dwFlags,BYTE *pbData, DWORD *pdwDataLen, DWORD dwBufLen) {
	int ret = 0;
	int timeout = 0;
	int comid;
	char *key;
	PHKEY_Z phKey = NULL;
	char *ip;
	int port;
	phKey = (PHKEY_Z)hKey;
	

	ip = getHsmIP();
	port = getHsmPORT();
	key = (CHAR*)phKey->key;
	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	if (NULL == phKey) {
		VarLogEntry(" CPEncryptImpl", "hKey error", -1, 0);
		return -1;
	}
	
	comid = InitHsmDevice(ip, port, timeout);
	if (comid < 0) {
		VarLogEntry(" CPEncryptImpl", "InitHsmDevice error", comid, 0);
		return comid;
	}
	/////////////////////////////////////
	__try {
		int algo = 0;
		int dataBlockFlag = 0;
		int encryptFlag = 0;
		int algoOperationMode = 0;
		int inputFormat = 1;
		int outputFormat = 1;
		char keyType[] = ZEK_TYPE;
		int paddingMode = 0;
		char paddingChar[] = "0000";
		int paddingFlag = 0;
		char *iv = NULL;
		int outFlag;
	
		ret = encryptDecrypt(comid, 0, NULL, algo,
		dataBlockFlag,
		encryptFlag,
		algoOperationMode,
		inputFormat,
		outputFormat,
		keyType,
		key,
		paddingMode,
		paddingChar,
		paddingFlag,
		iv,
		&outFlag,
		(int*)pdwDataLen,
		(CHAR*)pbData);
		if (ret < 0) {
			VarLogEntry(" CPEncryptImpl", "encryptDecrypt error", ret, 0);
			return ret;
		}
	}
	__finally
	{
		CloseHsmDevice(comid);
	}
	return ret;
}

int CPCreateHashImpl(HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey, DWORD dwFlags, HCRYPTHASH *phHash) {
	int ret = 0;
	int algo;
	PHHASH_Z phHash_z;
	/*
	 HASH_MD2        0
	 HASH_SHA1       1
	 HASH_MD5        2
	 HASH_ISO10118_2 3
	 HASH_NOHASH     4
	 HASH_SHA224     5
	 HASH_SHA256     6
	 HASH_SHA384     7
	 HASH_SHA512     8
	 HASH_MD4        9
	 HASH_RIPEMD128  10
	 HASH_RIPEMD160  11
	 HASH_RIPEMD256  12
	 HASH_RIPEMD320  13
	 HASH_SM3  14
	*/

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	switch (Algid)
	{
	case HASH_MD2:
		algo = HASH_MD2;
		break;
	case HASH_SHA1:
		algo = HASH_SHA1;
		break;
	case HASH_MD5:
		algo = HASH_MD5;
		break;
	case HASH_ISO10118_2:
		algo = HASH_ISO10118_2;
		break;
	case HASH_NOHASH:
		algo = HASH_NOHASH;
		break;
	case HASH_SHA224:
		algo = HASH_SHA224;
		break;
	case HASH_SHA256:
		algo = HASH_SHA256;
		break;
	case HASH_SHA384:
		algo = HASH_SHA384;
		break;
	case HASH_SHA512:
		algo = HASH_SHA512;
		break;
	case HASH_MD4:
		algo = HASH_MD4;
		break;
	case HASH_RIPEMD128:
		algo = HASH_RIPEMD128;
		break;
	case HASH_RIPEMD160:
		algo = HASH_RIPEMD160;
		break;
	case HASH_RIPEMD256:
		algo = HASH_RIPEMD256;
		break;
	case HASH_RIPEMD320:
		algo = HASH_RIPEMD320;
		break;
	case HASH_SM3:
		algo = HASH_SM3;
		break;
	default:
		VarLogEntry(" CPCreateHashImpl", "Algid error", Algid, 0);
		return  -1;
	}
	phHash_z = (PHHASH_Z)malloc(sizeof(HHASH_Z));
	if (NULL != phHash_z) {
		VarLogEntry(" CPCreateHashImpl", "Memory error",-1, 0);
		return -1;
	}
	phHash_z->phKey = (PHKEY_Z)hKey;
	memcpy(phHash_z->ALGID,"%02d",algo);
	*phHash = (HCRYPTHASH)phHash_z;
	return ret;
}

int CPGetHashParamImpl(HCRYPTPROV hProv, HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData, DWORD *pdwDataLen, DWORD dwFlags) {
	int ret = 0;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	/*
	HP_ALGID
	HP_HASHVAL
	HP_HASHSIZE
	HP_HMAC_INFO
	HP_TLS1PRF_LABEL
	HP_TLS1PRF_SEED
	*/
	PHHASH_Z phzHash = (PHHASH_Z)hHash;
	if (NULL == phzHash) {
		VarLogEntry(" CPGetHashParamImpl", "hHash error", -1, 0);
		return (-1);
	}
	switch (dwParam)
	{
	case HP_ALGID:
		*pdwDataLen = strlen((CHAR*)phzHash->ALGID);
		if (0 == *pdwDataLen) {
			VarLogEntry(" CPGetHashParamImpl", "dwParam Empty", -1, 0);
			return (-1);
		}
		if (NULL != pbData) {
			memcpy(pbData, phzHash->ALGID, *pdwDataLen);
		}
		break;
	case HP_HASHVAL:
		*pdwDataLen = strlen((CHAR*)phzHash->HASHVAL);
		if (0 == *pdwDataLen) {
			VarLogEntry(" CPGetHashParamImpl", "dwParam Empty", -1, 0);
			return (-1);
		}
		if (NULL != pbData) {
			memcpy(pbData, phzHash->HASHVAL, *pdwDataLen);
		}
		break;
	case HP_HASHSIZE:
		*pdwDataLen = strlen((CHAR*)phzHash->HASHSIZE);
		if (0 == *pdwDataLen) {
			VarLogEntry(" CPGetHashParamImpl", "dwParam Empty", -1, 0);
			return (-1);
		}
		if (NULL != pbData) {
			memcpy(pbData, phzHash->HASHSIZE, *pdwDataLen);
		}
		break;
	case HP_HMAC_INFO:
		*pdwDataLen = strlen((CHAR*)phzHash->HMAC_INFO);
		if (0 == *pdwDataLen) {
			VarLogEntry(" CPGetHashParamImpl", "dwParam Empty", -1, 0);
			return (-1);
		}
		if (NULL != pbData) {
			memcpy(pbData, phzHash->HMAC_INFO, *pdwDataLen);
		}
		break;
	case HP_TLS1PRF_LABEL:
		*pdwDataLen = strlen((CHAR*)phzHash->TLS1PRF_LABEL);
		if (0 == *pdwDataLen) {
			VarLogEntry(" CPGetHashParamImpl", "dwParam Empty", -1, 0);
			return (-1);
		}
		if (NULL != pbData) {
			memcpy(pbData, phzHash->TLS1PRF_LABEL, *pdwDataLen);
		}
		break;
	case HP_TLS1PRF_SEED:
		*pdwDataLen = strlen((CHAR*)phzHash->TLS1PRF_SEED);
		if (0 == *pdwDataLen) {
			VarLogEntry(" CPGetHashParamImpl", "dwParam Empty", -1, 0);
			return (-1);
		}
		if (NULL != pbData) {
			memcpy(pbData, phzHash->TLS1PRF_SEED, *pdwDataLen);
		}
		break;
	default:
		VarLogEntry(" CPGetHashParamImpl", "dwParam error", dwParam, 0);
		return (-1);
	}

	return ret;
}

int CPHashDataImpl(HCRYPTPROV hProv, HCRYPTHASH hHash, const BYTE *pbData, DWORD dwDataLen, DWORD dwFlags) {
	int ret = 0;
	int timeout = 0;
	int cmdid;
	char * ip = getHsmIP();
	int port = getHsmPORT();
	PHHASH_Z phzHash = (PHHASH_Z)hHash;
	int hash_id = atoi((CHAR*)phzHash->ALGID);
	UCHAR *data = (UCHAR*)pbData;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	//
	if (NULL == phzHash) {
		VarLogEntry(" CPHashDataImpl", "hHash error", -1, 0);
		return -1;
	}
	//
	cmdid = InitHsmDevice(ip, port, timeout);
	if (cmdid<0) {
		VarLogEntry(" CPHashDataImpl", "InitHsmDevice error",cmdid, 0);
		return (-1);
	}
	//
	ret = genhash(cmdid, 0, NULL, hash_id, dwDataLen, (UCHAR*)pbData, (UCHAR*)pbData);
	if (ret != 0) {
		CloseHsmDevice(cmdid);
		VarLogEntry(" CPHashDataImpl", "genhash error", ret, 0);
		return (ret);
	}
	//
	CloseHsmDevice(cmdid);
	return ret;
}

int CPSetHashParamImpl(HCRYPTPROV hProv, HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData, DWORD dwFlags) {
	int ret = 0;
	DWORD *pdwDataLen = 0;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	/*
	HP_ALGID
	HP_HASHVAL
	HP_HASHSIZE
	HP_HMAC_INFO
	HP_TLS1PRF_LABEL
	HP_TLS1PRF_SEED
	*/
	PHHASH_Z phzHash = (PHHASH_Z)hHash;
	if (NULL == phzHash) {
		VarLogEntry(" CPSetHashParamImpl", "hHash error", -1, 0);
		return (-1);
	}
	switch (dwParam)
	{
	case HP_ALGID:
		*pdwDataLen = strlen((CHAR*)pbData);
		if (0 == *pdwDataLen) {
			VarLogEntry(" CPSetHashParamImpl", "dwParam Empty", -1, 0);
			return (-1);
		}
		memcpy(phzHash->ALGID, pbData, *pdwDataLen);
		break;
	case HP_HASHVAL:
		*pdwDataLen = strlen((CHAR*)pbData);
		if (0 == *pdwDataLen) {
			VarLogEntry(" CPSetHashParamImpl", "dwParam Empty", -1, 0);
			return (-1);
		}
		memcpy(phzHash->HASHVAL, pbData, *pdwDataLen);
		break;
	case HP_HASHSIZE:
		*pdwDataLen = strlen((CHAR*)pbData);
		if (0 == *pdwDataLen) {
			VarLogEntry(" CPSetHashParamImpl", "dwParam Empty", -1, 0);
			return (-1);
		}
		memcpy(phzHash->HASHSIZE, pbData, *pdwDataLen);
		break;
	case HP_HMAC_INFO:
		*pdwDataLen = strlen((CHAR*)pbData);
		if (0 == *pdwDataLen) {
			VarLogEntry(" CPSetHashParamImpl", "dwParam Empty", -1, 0);
			return (-1);
		}
		memcpy(phzHash->HMAC_INFO, pbData, *pdwDataLen);
		break;
	case HP_TLS1PRF_LABEL:
		*pdwDataLen = strlen((CHAR*)pbData);
		if (0 == *pdwDataLen) {
			VarLogEntry(" CPSetHashParamImpl", "dwParam Empty", -1, 0);
			return (-1);
		}
		memcpy(phzHash->TLS1PRF_LABEL, pbData, *pdwDataLen);
		break;
	case HP_TLS1PRF_SEED:
		*pdwDataLen = strlen((CHAR*)pbData);
		if (0 == *pdwDataLen) {
			VarLogEntry(" CPSetHashParamImpl", "dwParam Empty", -1, 0);
			return (-1);
		}
		memcpy(phzHash->TLS1PRF_SEED, pbData, *pdwDataLen);
		break;
	default:
		VarLogEntry(" CPSetHashParamImpl", "dwParam error", dwParam, 0);
		return (-1);
	}
	return ret;
}

int CPSignHashImpl(HCRYPTPROV hProv, HCRYPTHASH hHash, DWORD dwKeySpec, LPCWSTR sDescription, DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen) {
	int comid;
	int ret = 0;
	PHKEY_Z phzKey;
	int timeout = 0;
	PHHASH_Z phzHash = NULL;
	
	char *ip = getHsmIP(); 
	int port = getHsmPORT();
	int hash_id = atoi((CHAR*)phzHash->ALGID);
	int data_length = *pdwSigLen;
	UCHAR * data = pbSignature;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	//hash对象
	phzHash = (PHHASH_Z)hHash;
	if (NULL == phzHash) {
		VarLogEntry(" CPSignHashImpl", "hHash error", -1, 0);
		return -1;
	}
	//密钥对象
	phzKey = phzHash->phKey;
	if (NULL == phzKey) {
		VarLogEntry(" CPSignHashImpl", "hKey error", -1, 0);
		return -1;
	}
	//////////////
	comid = InitHsmDevice(ip, port, timeout);
	if (comid < 0) {
		VarLogEntry(" CPSignHashImpl", "InitHsmDevice error", comid, 0);
		return (comid);
	}
	
	__try {
		int msghdlen = 0;
		char *msghd = NULL;
		int sign_id = 01;
		int pad_mode = 01;
		int mgfHash = NULL;
		int OAEP_parm_len = NULL;
		UCHAR *OAEP_parm = NULL;
		int pssRule = NULL;
		int trailerField = NULL;
		int index = 99;
		int authenDataLen = 0;
		UCHAR * authenData = NULL;

		ret = rsaprisign(comid, msghdlen, msghd, hash_id, sign_id,
			pad_mode,
			mgfHash,
			OAEP_parm_len,
			OAEP_parm,
			pssRule,
			trailerField,
			data_length,
			data,
			index,
			phzKey->pvLen,
			phzKey->pvKey,
			pbSignature,
			(int *)pdwSigLen);
		if (ret < 0) {
			VarLogEntry(" CPSignHashImpl", "rsaprisign error", ret, 0);
			return ret;
		}
	}
	__finally
	{
		CloseHsmDevice(comid);
	}
	return ret;
}

int CPVerifySignatureImpl(HCRYPTPROV hProv, HCRYPTHASH hHash, const BYTE *pbSignature, DWORD dwSigLen, HCRYPTKEY hPubKey, LPCWSTR sDescription, DWORD dwFlags) {
	int ret = 0;
	int timeout = 0;
	int comid;
	PHHASH_Z phzHash;
	PHKEY_Z phzKey;
	char * ip = getHsmIP();
	int port = getHsmPORT();

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	////////////////////////////////
	//hash对象
	phzHash = (PHHASH_Z)hHash;
	if (NULL == phzHash) {
		VarLogEntry(" CPVerifySignatureImpl", "hHash error", -1, 0);
		return -1;
	}
	//密钥对象
	phzKey = phzHash->phKey;
	if (NULL == phzKey) {
		VarLogEntry(" CPVerifySignatureImpl", "hKey error", -1, 0);
		return -1;
	}
	//////////////
	comid = InitHsmDevice(ip, port, timeout);
	if (comid < 0) {
		VarLogEntry(" CPVerifySignatureImpl", "InitHsmDevice error", comid, 0);
		return (comid);
	}
	__try {
		int msghdlen = 0;
		char *msghd = NULL;
		int hash_id = 01;
		int sign_id = 01;
		int pad_mode = 01;
		int mgfHash = NULL;
		int OAEP_parm_len = NULL;
		UCHAR *OAEP_parm = NULL;
		int pssRule = NULL;
		int trailerField = NULL;
		int index = 99;
		int authenDataLen = 0;
		UCHAR * authenData = NULL;
		///////////////////////////////////
		ret = rsapubverify(comid, msghdlen, msghd, hash_id, sign_id,
			pad_mode,
			mgfHash,
			OAEP_parm_len,
			OAEP_parm,
			pssRule,
			trailerField,
			dwSigLen,
			(UCHAR*)pbSignature,
			strlen((CHAR*)dwFlags),
			(UCHAR*)dwFlags,
			index,
			NULL,
			phzKey->puKey,
			phzKey->puLen,
			authenDataLen,
			authenData
		);
		if (0 != ret) {
			VarLogEntry(" CPVerifySignatureImpl", "rsapubverify error", ret, 0);
			return ret;
		}
	}
	__finally
	{
		CloseHsmDevice(comid);
	}
	////////////////////////////////
	return ret;
}

int CPHashSessionKeyImpl(HCRYPTPROV hProv, HCRYPTHASH hHash, HCRYPTKEY hKey, DWORD dwFlags) {
	int ret = 0;
	int timeout = 0;
	int cmdid;
	char * ip = getHsmIP();
	int port = getHsmPORT();
	PHHASH_Z phzHash = (PHHASH_Z)hHash;
	int hash_id = atoi((CHAR*)phzHash->ALGID);
	PHKEY_Z phzKey = (PHKEY_Z)hKey;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	//
	if (NULL == phzHash) {
		VarLogEntry(" CPHashSessionKeyImpl", "hHash NULL", -1, 0);
		return -1;
	}
	//
	if (NULL == phzKey) {
		VarLogEntry(" CPHashSessionKeyImpl", "hKey NULL", -1, 0);
		return -1;
	}
	//
	cmdid = InitHsmDevice(ip, port, timeout);
	if (cmdid<0) {
		VarLogEntry(" CPHashSessionKeyImpl", "InitHsmDevice error", cmdid, 0);
		return (-1);
	}
	//
	ret = genhash(cmdid, 0, NULL, hash_id, phzKey->len, phzKey->key, phzHash->keyHashValue);
	if (ret != 0) {
		CloseHsmDevice(cmdid);
		VarLogEntry(" CPHashSessionKeyImpl", "genhash error", ret, 0);
		return (ret);
	}
	//
	CloseHsmDevice(cmdid);
	return ret;
}
#endif

//1 cpAcquireContext
int cpAcquireContextImpl(HCRYPTPROV *phProv, CHAR *pszContainer, DWORD dwFlags, PVTableProvStruc pVTable){
	int ret=0;

	//加载配置文件
	if ((ret = initCSP())<0) {
		LogEntry("cpAcquireContext", "initCSP error", ret, 0);
		return ret;
	}
	//加密机状态
	if ((ret = testSjl22()) != 0) {
		LogEntry("cpAcquireContext", "testSjl22 error", ret, 0);
		return ret;
	}
	*phProv = getMutexFlag();
	return ret;
}

//2 cpGetProvParam
int   cpGetProvParamImpl(HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData, DWORD *pdwDataLen, DWORD dwFlags){
	int ret = 0;
	HKEY hKey;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	//获取注册表属性
	ret = GMN_RegOpen(&hKey);
	if (ERROR_SUCCESS != ret) {
		VarLogEntry(" GMN_RegOpen", "error:", ret, 0);
		return ret;
	}
	__try {
		//获取属性
		ret = GMN_RegQueryValueEx(hKey, (CHAR*)dwParam, 0, &dwFlags, pbData, pdwDataLen);
		if (ERROR_SUCCESS != ret) {
			VarLogEntry(" GMN_RegQueryValueEx", "error: ret", ret, 0);
			VarLogEntry(" GMN_RegQueryValueEx", "key: %s", -1, 0,
				(CHAR*)dwParam);
			return ret;
		}
	}
	__finally {
		GMN_RegCloseKey(hKey);
	}
	return ret;
}

//3 cpReleaseContext
int   cpReleaseContextImpl(HCRYPTPROV hProv, DWORD dwFlags){
	int ret = 0;
	//结束线程同步
	ret = CSP_Destroy_Mutex();
	hProv = 0;
	return ret;
}

//4 cpSetProvParam
int   cpSetProvParamImpl(HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData, DWORD dwFlags){
	int ret = 0;
	HKEY hKey;
	
	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	//获取注册表属性
	ret = GMN_RegOpen(&hKey);
	if (ERROR_SUCCESS != ret) {
		VarLogEntry(" GMN_RegOpen", "error", ret, 0);
		return ret;
	}
	__try {
		//设置注册表属性
		ret = GMN_RegSetValueEx(hKey, (LPCSTR)dwParam, 0,
			(NULL == dwFlags ? REG_SZ : dwFlags),
			pbData, strlen((char*)pbData));
		if (ERROR_SUCCESS != ret) {
			VarLogEntry(" GMN_RegSetValueEx", "error", ret, 0);
			return ret;
		}
	
	}
	__finally {
		GMN_RegCloseKey(hKey);
	}
	return ret;
}

//5 cpDeriveKey
int   cpDeriveKeyImpl(HCRYPTPROV hProv, ALG_ID Algid, HCRYPTHASH hBaseData, DWORD dwFlags, HCRYPTKEY *phKey){
	int ret = 0;
	///////////////////////////
	int timeout = 0;
	int comid;
	CHAR cKey[256];
	UCHAR deriveKey[256];
	UCHAR checkValue[256];
	HKEY_Z *hKey_z = (HKEY_Z*)phKey;
	char *key = (CHAR*)hKey_z->key;
	int algo = Algid;
	char *data = (char*)hBaseData;
	int dataLen = strlen(data);
	int keyLen = strlen(key) / 2;
	int derivationmode = 0;
	int encmode = 0;
	char deriveKeyType[] = ZMK_TYPE;
	char derivationKeyType[] = ZMK_TYPE;
	char *iv = NULL;
	int deriveKeyLen;
	HKEY_Z *hKey_deri;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	comid = InitHsmDevice(getHsmIP(), getHsmPORT(), timeout);
	if (comid<0) {
		VarLogEntry(" InitHsmDevice", "connect error", comid, 0);
		return comid;
	}
	__try {
		PackBCD(key, (unsigned char*)cKey, strlen(key));
		ret = derivatekey(comid, 0, NULL, algo, derivationmode, encmode, deriveKeyType, derivationKeyType, keyLen, cKey, dataLen, iv, data, 0, NULL, NULL, (char*)deriveKey, (char*)checkValue);
		if (ret<0) {
			VarLogEntry("derivatekey", "error", ret, 0);
			return ret;
		}
		deriveKeyLen = strlen((CHAR*)deriveKey);

		hKey_deri = (HKEY_Z *)malloc(sizeof(HKEY_Z));
		if (NULL == hKey_deri) {
			VarLogEntry("cpDeriveKey", "memory error", -1, 0);
			return -1;
		}

		hKey_deri->len = deriveKeyLen;
		memcpy(hKey_deri->key, deriveKey, deriveKeyLen);
		memcpy(hKey_deri->key, checkValue, strlen((CHAR*)checkValue));
		sprintf((CHAR*)hKey_deri->ALGID, "%02d", algo);
		sprintf((CHAR*)hKey_deri->KEYLEN, "%04d", deriveKeyLen);
		*phKey = (LONG)hKey_deri;
	}
	__finally
	{
		CloseHsmDevice(comid);
	}
	//////////////////////////
	return ret;
}

//6 cpDestroyKey
int   cpDestroyKeyImpl(HCRYPTPROV hProv, HCRYPTKEY hKey){
	int ret = 0;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	if (NULL != hKey) {
		free((void*)hKey);
		hKey = NULL;
	}
	return ret;
}

//7 cpExportKey
int   cpExportKeyImpl(HCRYPTPROV hProv, HCRYPTKEY hKey, HCRYPTKEY hPubKey, DWORD dwBlobType, 
	DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen){
	int ret = 0;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	ret = exportrsadeskeyImpl(hKey, hPubKey, pbData, (int *)pdwDataLen);
	if (ret<0 || pdwDataLen<0) {
		VarLogEntry("cpExportKey", "exportrsadeskeyImpl error", ret, 0);
		return ret;
	}
	return ret;
}

//8 cpGenKey
int   cpGenKeyImpl(HCRYPTPROV hProv, ALG_ID Algid, DWORD dwFlags, HCRYPTKEY *phKey){
	int ret = 0;
	int timeout = 0;
	int comid;
	PHKEY_Z hKey = NULL;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	__try {
		comid = InitHsmDevice(getHsmIP(), getHsmPORT(), timeout);
		if (comid<0) {
			VarLogEntry(" InitHsmDevice", "connect error", comid, 0);
			return comid;
		}
		
		switch (Algid)
		{
		case ALGO_DESTDES:
		case ALGO_SSF33:
		case ALGO_SSF10:
		case ALGO_SCB2:
		case ALGO_SM4:
			ret = generateKeyImpl(comid, hKey,Algid);
			if (ret<0) {
				return ret;
			}
			break;

		case ALGO_RSA:
			ret = genrsakeyImpl(hProv, dwFlags, hKey, comid,  Algid);
			if (ret < 0) {
				return ret;
			}
			break;
		case ALGO_SM2:
			ret = genSm2Key(hKey, comid, Algid);
			if (ret < 0) {
				return ret;
			}
			break;
		default:
			VarLogEntry(" cpGenKey", "Algid error", Algid, 0);
			return -1;
		}
		*phKey = (LONG)hKey;
	}
	__finally {
		CloseHsmDevice(comid);
	}
	return ret;
}

//9 cpGenRandom
int   cpGenRandomImpl(HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer){
	int comid;
	int ret = 0;
	int timeout = 0;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	////////////////////////////
	comid = InitHsmDevice(getHsmIP(), getHsmPORT(), timeout);
	if (comid<0) {
		VarLogEntry(" InitHsmDevice", "connect error", comid, 0);
		return comid;
	}

	__try {
		ret = genrandom(comid, 0, NULL, dwLen, pbBuffer);
		if (ret<0) {
			VarLogEntry(" genrandom", "error", ret, 0);
			return ret;
		}
	}
	__finally {
		CloseHsmDevice(comid);
	}
	///////////////////////////
	return ret;
}

//10 cpGetKeyParam
int   cpGetKeyParamImpl(HCRYPTPROV hProv, HCRYPTKEY hKey, DWORD dwParam, LPBYTE pbData, LPDWORD pcbDataLen, DWORD dwFlags){
	int ret = 0;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	/////
	HKEY_Z * tmpKey = (HKEY_Z *)hKey;
	if (tmpKey == NULL) {
		VarLogEntry(" cpGetKeyParam", "hKey == NULL error", -1, 0);
		return -1;
	}

	return  getKeyParam( dwParam, tmpKey,  pbData,  pcbDataLen);
}

//11 cpGetUserKey
int   cpGetUserKeyImpl(HCRYPTPROV hProv, DWORD dwKeySpec, HCRYPTKEY *phUserKey){
	int ret = 0;
	HKEY_Z * hzKey;
	
	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}

	DWORD dwKeyLen = sizeof(HKEY_Z);
	hzKey = (PHKEY_Z)malloc(dwKeyLen);
	if (NULL == hzKey) {
		LogEntry("cpGetUserKey", "Memory error", -1, 0);
		return -1;
	}
	/*
	从密钥容器中获取已持久化的用户密钥句柄:从容器中获取RSA的密钥文件ID标识和。
	dwKeySpec phUserKey 根据密钥属性获取密钥句柄
	1. AT_KEYEXCHANGE 交换密钥
	2. AT_SIGNATURE 签名密钥
	*/
	switch (dwKeySpec)
	{
	case AT_KEYEXCHANGE:
		ret = cpGetProvParamImpl(hProv, AT_KEYEXCHANGE, (BYTE*)hzKey, &dwKeyLen, REG_BINARY);
		if (ERROR_SUCCESS != ret || dwKeyLen <= 0) {
			free(hzKey);
			LogEntry("cpGetUserKey", "AT_KEYEXCHANGE error", ret, 0);
			return -1;
		}
		break;
	case AT_SIGNATURE:
		ret = cpGetProvParamImpl(hProv, AT_SIGNATURE, (BYTE *)hzKey, &dwKeyLen, REG_BINARY);
		if (ERROR_SUCCESS != ret || dwKeyLen <= 0) {
			free(hzKey);
			LogEntry("cpGetUserKey", "AT_SIGNATURE error", ret, 0);
			return -1;
		}
		break;
	default:
		free(hzKey);
		LogEntry("cpGetUserKey", "dwKeySpec error", dwKeySpec, 0);
		return -1;
	}
	phUserKey = (HCRYPTKEY *)hzKey;
	return ret;
}

//12 cpImportKey
int   cpImportKeyImpl(HCRYPTPROV hProv, const BYTE *pbData, DWORD dwDataLen, HCRYPTKEY hPubKey, DWORD dwFlags, HCRYPTKEY *phKey){
	int ret = 0;
	HPKEY_Z * pKey;
	HKEY_Z * hKey;
	UCHAR wkLmk[255];
	int keylen;
	UCHAR cv[64];

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	pKey = (HPKEY_Z*)hPubKey;
	ret = importrsadeskeyImpl((UCHAR *)pbData, dwDataLen, pKey->pvKey, pKey->pvLen, wkLmk, &keylen, cv);
	if (ret != 0) {
		VarLogEntry(" importrsadeskeyImplvoid", "error", ret, 0);
		return ret;
	}
	hKey = (HKEY_Z *)malloc(sizeof(HKEY_Z));
	if (NULL == hKey) {
		VarLogEntry(" cpImportKey", "memory error", -1, 0);
		return -1;
	}
	hKey->len = keylen;
	memcpy(hKey->key, wkLmk, keylen);
	memcpy(hKey->cv, cv, strlen((CHAR*)cv));
	*phKey = (LONG)hKey;
	return ret;
}

//13 cpSetKeyParam
int   cpSetKeyParamImpl(HCRYPTPROV hProv, HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData, DWORD dwFlags){
	int ret = 0;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	PHKEY_Z phKey = (PHKEY_Z)hKey;
	if (phKey == NULL) {
		VarLogEntry(" cpSetKeyParamImpl", "Memory error", -1, 0);
		return -1;
	}

	switch (dwParam)
	{
	case KP_ALGID:
		memcpy(phKey->ALGID, pbData, sizeof(pbData));
		break;
	case KP_BLOCKLEN:
		memcpy(phKey->BLOCKLEN, pbData, sizeof(pbData));
		break;
	case KP_KEYLEN:
		memcpy(phKey->KEYLEN, pbData, sizeof(pbData));
		break;
	case KP_SALT:
		memcpy(phKey->SALT, pbData, sizeof(pbData));
		break;
	case KP_PERMISSIONS:
		memcpy(phKey->PERMISSIONS, pbData, sizeof(pbData));
		break;
	case KP_IV:
		memcpy(phKey->IV, pbData, sizeof(pbData));
		break;
	case KP_PADDING:
		memcpy(phKey->PADDING, pbData, sizeof(pbData));
		break;
	case KP_MODE:
		memcpy(phKey->MODE, pbData, sizeof(pbData));
		break;
	case KP_MODE_BITS:
		memcpy(phKey->MODE_BITS, pbData, sizeof(pbData));
		break;
	case KP_EFFECTIVE_KEYLEN:
		memcpy(phKey->EFFECTIVE_KEYLEN, pbData, sizeof(pbData));
		break;
	default:
		VarLogEntry(" cpSetKeyParamImpl", "dwParam error", dwParam, 0);
		return -1;
	}
	return ret;
}

//14 cpDecrypt
int   cpDecryptImpl(HCRYPTPROV hProv, HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen){
	int ret = 0;
	int timeout = 0;
	int comid;
	PHKEY_Z phKey = NULL;
	char *ip;
	int port;


	ip = getHsmIP();
	port = getHsmPORT();

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	phKey = (PHKEY_Z)hKey;
	if (NULL == phKey) {
		VarLogEntry(" cpDecryptImpl", "hKey error", -1, 0);
		return -1;
	}
	comid = InitHsmDevice(ip, port, timeout);
	if (comid < 0) {
		VarLogEntry(" cpDecryptImpl", "InitHsmDevice error", comid, 0);
		return comid;
	}
	/////////////////////////////////////
	__try {
		ret = decryptAlgo( comid,  phKey, pbData, pdwDataLen);
		if (ret < 0) {
			VarLogEntry(" cpDecryptImpl", "decryptAlgo error", ret, 0);
		}
	}
	__finally
	{
		CloseHsmDevice(comid);
	}
	return ret;
}

//15 cpEncrypt
int   cpEncryptImpl(HCRYPTPROV hProv, HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, DWORD dwFlags, BYTE *pbData, 
	DWORD *pdwDataLen, DWORD dwBufLen){
	int ret = 0;
	int timeout = 0;
	int comid;
	PHKEY_Z phKey = NULL;
	char *ip;
	int port;
	phKey = (PHKEY_Z)hKey;


	ip = getHsmIP();
	port = getHsmPORT();
	
	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	if (NULL == phKey) {
		VarLogEntry(" cpEncryptImpl", "hKey error", -1, 0);
		return -1;
	}

	comid = InitHsmDevice(ip, port, timeout);
	if (comid < 0) {
		VarLogEntry(" cpEncryptImpl", "InitHsmDevice error", comid, 0);
		return comid;
	}
	/////////////////////////////////////
	__try {
		ret = encryptAlgo(comid, phKey, pbData, pdwDataLen);
		if (ret < 0) {
			VarLogEntry(" cpDecryptImpl", "decryptAlgo error", ret, 0);
		}
	}
	__finally
	{
		CloseHsmDevice(comid);
	}
	return ret;
}

//16 cpCreateHash
int   cpCreateHashImpl(HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey, DWORD dwFlags, HCRYPTHASH *phHash){
	int ret = 0;
	int algo;
	PHHASH_Z phHash_z;
	/*
	HASH_MD2        0
	HASH_SHA1       1
	HASH_MD5        2
	HASH_ISO10118_2 3
	HASH_NOHASH     4
	HASH_SHA224     5
	HASH_SHA256     6
	HASH_SHA384     7
	HASH_SHA512     8
	HASH_MD4        9
	HASH_RIPEMD128  10
	HASH_RIPEMD160  11
	HASH_RIPEMD256  12
	HASH_RIPEMD320  13
	HASH_SM3  14
	*/

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	switch (Algid)
	{
	case HASH_MD2:
		algo = HASH_MD2;
		break;
	case HASH_SHA1:
		algo = HASH_SHA1;
		break;
	case HASH_MD5:
		algo = HASH_MD5;
		break;
	case HASH_ISO10118_2:
		algo = HASH_ISO10118_2;
		break;
	case HASH_NOHASH:
		algo = HASH_NOHASH;
		break;
	case HASH_SHA224:
		algo = HASH_SHA224;
		break;
	case HASH_SHA256:
		algo = HASH_SHA256;
		break;
	case HASH_SHA384:
		algo = HASH_SHA384;
		break;
	case HASH_SHA512:
		algo = HASH_SHA512;
		break;
	case HASH_MD4:
		algo = HASH_MD4;
		break;
	case HASH_RIPEMD128:
		algo = HASH_RIPEMD128;
		break;
	case HASH_RIPEMD160:
		algo = HASH_RIPEMD160;
		break;
	case HASH_RIPEMD256:
		algo = HASH_RIPEMD256;
		break;
	case HASH_RIPEMD320:
		algo = HASH_RIPEMD320;
		break;
	case HASH_SM3:
		algo = HASH_SM3;
		break;
	default:
		VarLogEntry(" cpCreateHashImpl", "Algid error", Algid, 0);
		return  -1;
	}
	phHash_z = (PHHASH_Z)malloc(sizeof(HHASH_Z));
	if (NULL != phHash_z) {
		VarLogEntry(" cpCreateHashImpl", "Memory error", -1, 0);
		return -1;
	}
	phHash_z->phKey = (PHKEY_Z)hKey;
	memcpy(phHash_z->ALGID, "%02d", algo);
	*phHash = (HCRYPTHASH)phHash_z;
	return ret;
}

//17 cpDestroyHash
int   cpDestroyHashImpl(HCRYPTPROV hProv, HCRYPTHASH hHash){
	int ret = 0;
	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}

	if (NULL != hHash) {
		free((void *)hHash);
	}
	return ret;
}

//18 cpDuplicateHash 附加函数
int   cpDuplicateHashImpl(HCRYPTPROV hProv, HCRYPTHASH hHash, DWORD *pdwReserved, DWORD dwFlags, HCRYPTHASH *phHash){
	int ret = 0;
	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	if (NULL == (PHHASH_Z)hHash) {
		LogEntry("cpDuplicateHash", "NULL == (PHHASH_Z)hHash", -1, 0);
		return -1;
	}
	PHHASH_Z phzHash;
	phzHash = (PHHASH_Z)malloc(sizeof(HHASH_Z));
	if (NULL == phzHash) {
		LogEntry("cpDuplicateHash", "Memory error", -1, 0);
		return -1;
	}
	memcpy(phzHash, (PHHASH_Z)hHash, sizeof(HHASH_Z));
	*phHash = (HCRYPTHASH)phzHash;
	return ret;
}

//19 cpGetHashParam
int   cpGetHashParamImpl(HCRYPTPROV hProv, HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData, DWORD *pdwDataLen, DWORD dwFlags){
	int ret = 0;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	PHHASH_Z phzHash = (PHHASH_Z)hHash;
	if (NULL == phzHash) {
		VarLogEntry(" cpGetHashParamImpl", "hHash error", -1, 0);
		return (-1);
	}
	return getHashParam( dwParam,  phzHash,  pbData, pdwDataLen);
}

//20 cpHashData
int   cpHashDataImpl(HCRYPTPROV hProv, HCRYPTHASH hHash, const BYTE *pbData, DWORD dwDataLen, DWORD dwFlags){
	int ret = 0;
	int timeout = 0;
	int cmdid;
	char * ip = getHsmIP();
	int port = getHsmPORT();
	PHHASH_Z phzHash = (PHHASH_Z)hHash;
	int hash_id = atoi((CHAR*)phzHash->ALGID);
	UCHAR *data = (UCHAR*)pbData;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	//
	if (NULL == phzHash) {
		VarLogEntry(" cpHashDataImpl", "hHash error", -1, 0);
		return -1;
	}
	//
	cmdid = InitHsmDevice(ip, port, timeout);
	if (cmdid<0) {
		VarLogEntry(" cpHashDataImpl", "InitHsmDevice error", cmdid, 0);
		return (-1);
	}
	__try {
		//
		ret = genhash(cmdid, 0, NULL, hash_id, dwDataLen, (UCHAR*)pbData, (UCHAR*)pbData);
		if (ret != 0) {
			CloseHsmDevice(cmdid);
			VarLogEntry(" cpHashDataImpl", "genhash error", ret, 0);
			return (ret);
		}
	}
	__finally {
		//
		CloseHsmDevice(cmdid);
	}
	return ret;
}

//21 cpSetHashParam
int   cpSetHashParamImpl(HCRYPTPROV hProv, HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData, DWORD dwFlags){
	int ret = 0;
	DWORD pdwDataLen = 0;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	PHHASH_Z phzHash = (PHHASH_Z)hHash;
	if (NULL == phzHash) {
		VarLogEntry(" cpSetHashParamImpl", "hHash error", -1, 0);
		return (-1);
	}
	
	pdwDataLen = strlen((CHAR*)pbData);
	if (0 == pdwDataLen) {
		VarLogEntry(" cpSetHashParamImpl", "pbData Empty error", -1, 0);
		return (-1);
	}

	/*
	HP_ALGID
	HP_HASHVAL
	HP_HASHSIZE
	HP_HMAC_INFO
	HP_TLS1PRF_LABEL
	HP_TLS1PRF_SEED
	*/
	switch (dwParam)
	{
	case HP_ALGID:
		memcpy(phzHash->ALGID, pbData, pdwDataLen);
		break;
	case HP_HASHVAL:
		memcpy(phzHash->HASHVAL, pbData, pdwDataLen);
		break;
	case HP_HASHSIZE:
		memcpy(phzHash->HASHSIZE, pbData, pdwDataLen);
		break;
	case HP_HMAC_INFO:
		memcpy(phzHash->HMAC_INFO, pbData, pdwDataLen);
		break;
	case HP_TLS1PRF_LABEL:
		memcpy(phzHash->TLS1PRF_LABEL, pbData, pdwDataLen);
		break;
	case HP_TLS1PRF_SEED:
		memcpy(phzHash->TLS1PRF_SEED, pbData, pdwDataLen);
		break;
	default:
		VarLogEntry(" cpSetHashParamImpl", "dwParam error", dwParam, 0);
		return (-1);
	}
	return ret;
}

//22 cpSignHash
int   cpSignHashImpl(HCRYPTPROV hProv, HCRYPTHASH hHash, DWORD dwKeySpec, LPCWSTR sDescription, 
	DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen){
	int comid;
	int ret = 0;
	PHKEY_Z phzKey;
	int timeout = 0;
	PHHASH_Z phzHash = NULL;

	char *ip = getHsmIP();
	int port = getHsmPORT();
	int hash_id = atoi((CHAR*)phzHash->ALGID);
	int data_length = *pdwSigLen;
	UCHAR * data = pbSignature;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	//hash对象
	phzHash = (PHHASH_Z)hHash;
	if (NULL == phzHash) {
		VarLogEntry(" cpSignHashImpl", "hHash error", -1, 0);
		return -1;
	}
	//密钥对象
	phzKey = phzHash->phKey;
	if (NULL == phzKey) {
		VarLogEntry(" cpSignHashImpl", "hKey error", -1, 0);
		return -1;
	}
	//////////////
	comid = InitHsmDevice(ip, port, timeout);
	if (comid < 0) {
		VarLogEntry(" cpSignHashImpl", "InitHsmDevice error", comid, 0);
		return (comid);
	}

	__try {
		int msghdlen = 0;
		char *msghd = NULL;
		int sign_id = 01;
		int pad_mode = 01;
		int mgfHash = NULL;
		int OAEP_parm_len = NULL;
		UCHAR *OAEP_parm = NULL;
		int pssRule = NULL;
		int trailerField = NULL;
		int index = 99;
		int authenDataLen = 0;
		UCHAR * authenData = NULL;

		ret = rsaprisign(comid, msghdlen, msghd, hash_id, sign_id,
			pad_mode,
			mgfHash,
			OAEP_parm_len,
			OAEP_parm,
			pssRule,
			trailerField,
			data_length,
			data,
			index,
			phzKey->pvLen,
			phzKey->pvKey,
			pbSignature,
			(int *)pdwSigLen);
		if (ret < 0) {
			VarLogEntry(" cpSignHashImpl", "rsaprisign error", ret, 0);
			return ret;
		}
	}
	__finally
	{
		CloseHsmDevice(comid);
	}
	return ret;
}

//23 cpVerifySignature
int   cpVerifySignatureImpl(HCRYPTPROV hProv, HCRYPTHASH hHash, const BYTE *pbSignature, DWORD dwSigLen, 
	HCRYPTKEY hPubKey, LPCWSTR sDescription, DWORD dwFlags){
	int ret = 0;
	int timeout = 0;
	int comid;
	PHHASH_Z phzHash;
	PHKEY_Z phzKey;
	char * ip = getHsmIP();
	int port = getHsmPORT();

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	////////////////////////////////
	//hash对象
	phzHash = (PHHASH_Z)hHash;
	if (NULL == phzHash) {
		VarLogEntry(" cpVerifySignatureImpl", "hHash error", -1, 0);
		return -1;
	}
	//密钥对象
	phzKey = phzHash->phKey;
	if (NULL == phzKey) {
		VarLogEntry(" cpVerifySignatureImpl", "hKey error", -1, 0);
		return -1;
	}
	//////////////
	comid = InitHsmDevice(ip, port, timeout);
	if (comid < 0) {
		VarLogEntry(" cpVerifySignatureImpl", "InitHsmDevice error", comid, 0);
		return (comid);
	}
	__try {
		int msghdlen = 0;
		char *msghd = NULL;
		int hash_id = 01;
		int sign_id = 01;
		int pad_mode = 01;
		int mgfHash = NULL;
		int OAEP_parm_len = NULL;
		UCHAR *OAEP_parm = NULL;
		int pssRule = NULL;
		int trailerField = NULL;
		int index = 99;
		int authenDataLen = 0;
		UCHAR * authenData = NULL;
		///////////////////////////////////
		ret = rsapubverify(comid, msghdlen, msghd, hash_id, sign_id,
			pad_mode,
			mgfHash,
			OAEP_parm_len,
			OAEP_parm,
			pssRule,
			trailerField,
			dwSigLen,
			(UCHAR*)pbSignature,
			strlen((CHAR*)dwFlags),
			(UCHAR*)dwFlags,
			index,
			NULL,
			phzKey->puKey,
			phzKey->puLen,
			authenDataLen,
			authenData
		);
		if (0 != ret) {
			VarLogEntry(" cpVerifySignatureImpl", "rsapubverify error", ret, 0);
			return ret;
		}
	}
	__finally
	{
		CloseHsmDevice(comid);
	}
	////////////////////////////////
	return ret;
}

//24 cpDuplicateKey 附加函数
int   cpDuplicateKeyImpl(HCRYPTPROV hUID, HCRYPTKEY hKey, DWORD *pdwReserved, DWORD dwFlags, HCRYPTKEY *phKey){
	int ret = 0;
	PHKEY_Z phzKey, dupKey;
	int keyLen = sizeof(HKEY_Z);
	
	//容器是否初始化
	ret = initJudgment(hUID);
	if (ret != 0) {
		return ret;
	}
	//密钥对象
	phzKey = (PHKEY_Z)hKey;
	if (NULL == phzKey) {
		VarLogEntry(" cpDuplicateKey", "hKey error", -1, 0);
		return -1;
	}
	dupKey = (PHKEY_Z)malloc(keyLen);
	if (NULL == dupKey) {
		VarLogEntry(" cpDuplicateKey", "Memory error", -1, 0);
		return -1;
	}
	memcpy(dupKey, phzKey, keyLen);
	*phKey = (HCRYPTKEY)dupKey;
	return ret;
}

//25 cpHashSessionKey
int   cpHashSessionKeyImpl(HCRYPTPROV hProv, HCRYPTHASH hHash, HCRYPTKEY hKey, DWORD dwFlags){
	int ret = 0;
	int timeout = 0;
	int cmdid;
	char * ip = getHsmIP();
	int port = getHsmPORT();
	PHHASH_Z phzHash = (PHHASH_Z)hHash;
	int hash_id = atoi((CHAR*)phzHash->ALGID);
	PHKEY_Z phzKey = (PHKEY_Z)hKey;

	//容器是否初始化
	ret = initJudgment(hProv);
	if (ret != 0) {
		return ret;
	}
	//
	if (NULL == phzHash) {
		VarLogEntry(" cpHashSessionKeyImpl", "hHash NULL", -1, 0);
		return -1;
	}
	//
	if (NULL == phzKey) {
		VarLogEntry(" cpHashSessionKeyImpl", "hKey NULL", -1, 0);
		return -1;
	}
	//
	cmdid = InitHsmDevice(ip, port, timeout);
	if (cmdid<0) {
		VarLogEntry(" cpHashSessionKeyImpl", "InitHsmDevice error", cmdid, 0);
		return (-1);
	}
	__try {
		//
		ret = genhash(cmdid, 0, NULL, hash_id, phzKey->len, phzKey->key, phzHash->keyHashValue);
		if (ret != 0) {
			CloseHsmDevice(cmdid);
			VarLogEntry(" CPHashSessionKeyImpl", "genhash error", ret, 0);
			return (ret);
		}
	}
	__finally {
		//
		CloseHsmDevice(cmdid);
	}
	return ret;
}


