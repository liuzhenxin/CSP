#include "stdafx.h"

int initCSP(void){
	char *value = NULL;
	int rv = -1;

	rv = GetConfigString("GMNCSP-DLL", "LoggingLevel", &value);
	if (rv != 0){
		return rv;
	}
	//printf("LoggingLevel ： %s\n",value);
	setLevel(atoi(value));
	if (value != NULL){
		free(value);
		value = NULL;
	}

	rv = GetConfigString("HSM-TOKEN", "IP", &value);
	if (rv != 0){
		return rv;
	}
	//printf("IP ： %s\n", value);
	setHsmIP(value);
	if (value != NULL){
		free(value);
		value = NULL;
	}

	rv = GetConfigString("HSM-TOKEN", "PORT", &value);
	if (rv != 0){
		return rv;
	}
	//printf("PORT ： %s\n", value);
	setHsmPORT(atoi(value));
	if (value != NULL){
		free(value);
		value = NULL;
	}
	
	rv = GetConfigString("HSM-TOKEN", "CV", &value);
	if (rv != 0){
		return rv;
	}
	//printf("CV ： %s\n", value);
	setHsmCV(value);
	if (value != NULL){
		free(value);
		value = NULL;
	}
	
	VarLogEntry("initCSP", "IP:%s PORT:%d Level:%d CV:%s", rv, 1,
		getHsmIP(), getHsmPORT(), getLevel(), getHsmCV());

	/*
	printf("LoggingLevel ： %d\n",getLevel());
	printf("IP ： %s\n", getIP());
	printf("PORT ： %d\n", getPORT());
	printf("CV ： %s\n", getCV());
	*/

	return rv;
}

int testSjl22(void){
	int ret = -1;
	int cmdid;

	//开启连接
	cmdid = InitHsmDevice(getHsmIP(),getHsmPORT(),0);
	if (cmdid < 0) {
		VarLogEntry("InitHsmDevice", " error IP:%s PORT:%d", cmdid, 0,
			getHsmIP(), getHsmPORT());
		return cmdid;
	}

	ret = testHSM(cmdid, 0, NULL, getHsmCV(), NULL);
	if (ret != 0) {
		VarLogEntry("InitHsmDevice", " error CHECKVALUE: %s", cmdid, 0,
			getHsmCV());
		return ret;
	}

	//关闭连接
	CloseHsmDevice(cmdid);
	return ret;
}

//RSA
int genrsakeyImpl(HCRYPTPROV hProv,DWORD dwFlags, PHPKEY_Z pKey, int comid, int Algid) {
	int key_usage = 2;
	int mode_flag = 0;
	int key_length = 2048;
	int public_exponent_len = 32;
	UCHAR public_exponent[] = { 0x00,0x01,0x00,0x01 };
	UCHAR public_key[4096];
	int  public_key_len;
	UCHAR mac[16];
	UCHAR private_key[4096];
	int  private_key_len;
	int ret = 0;

	int public_key_encoding = 1;
	ret = genrsakey(comid, 0, NULL,
		key_usage, mode_flag, 
		key_length, public_key_encoding,
		public_exponent_len,public_exponent,
		99,0,NULL,
		public_key,&public_key_len,mac,
		private_key,&private_key_len,
		NULL, NULL, NULL, NULL,NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,NULL, NULL, NULL, NULL);
	
	if (ret < 0 ) {
		VarLogEntry("CPGenKey", "RSA GENERATE error", ret, 0);
		return ret;
	}

	pKey = (HPKEY_Z*)malloc(sizeof(HPKEY_Z));
	if (NULL == pKey) {
		VarLogEntry("CPGenKey", "memory error", -1, 0);
		return -1;
	}

	pKey->puLen = public_key_len;
	pKey->pvLen = private_key_len;
	memcpy(pKey->puKey, public_key, public_key_len);
	memcpy(pKey->pvKey, private_key, private_key_len);
	sprintf((CHAR*)pKey->ALGID, "%02d",  Algid);

	/*
	从密钥容器中获取已持久化的用户密钥句柄:从容器中获取RSA的密钥文件ID标识和。
	dwKeySpec phUserKey 根据密钥属性获取密钥句柄
	1. AT_KEYEXCHANGE 交换密钥
	2. AT_SIGNATURE 签名密钥
	*/
	if (NULL != dwFlags) {
		switch (dwFlags)
		{
		case AT_KEYEXCHANGE:
			ret = cpSetProvParamImpl( hProv,AT_KEYEXCHANGE, (BYTE *)pKey, REG_BINARY);
			if (ERROR_SUCCESS != ret) {
				VarLogEntry(" genrsakeyImpl", "AT_KEYEXCHANGE error", ret, 0);
			}
			break;
		case AT_SIGNATURE:
			ret = cpSetProvParamImpl( hProv,AT_SIGNATURE, (BYTE *)pKey, REG_BINARY);
			if (ERROR_SUCCESS != ret) {
				VarLogEntry(" genrsakeyImpl", "AT_SIGNATURE error", ret, 0);
			}
			break;
		default:
			break;
		}
	}

	return ret;
}	

int exportrsadeskeyImpl(HCRYPTKEY hKey, HCRYPTKEY hPubKey, UCHAR * data,int * data_length) {
	char * ip = getHsmIP();
	int port = getHsmPORT();
	int timeout = 0;
	int comid;
	int ret;

	comid = InitHsmDevice(ip, port, timeout);
	if (comid < 0) {
		return comid;
	}
	//////////////////////////
	UCHAR * wkLmk = (UCHAR*)hKey;
	UCHAR * public_key = (UCHAR *)hPubKey;
	int  public_key_len = strlen((CHAR *)hPubKey);
	///////////////////////////
	int msghdlen = 0;
	char * msghd = NULL;
	int algo = 0;
	int sig_alg = 01;
	int pad_mode = 04;
	int mgfHash = NULL;
	int OAEP_parm_len = NULL;
	UCHAR *OAEP_parm = NULL;
	int keyBlockType = 3;
	int keyBlockTemplateLen = NULL;
	UCHAR *keyBlockTemplate = NULL;
	int keyOffset = NULL;
	int chkLen = NULL;
	int chkOffset = NULL;
	int keyLen = 16;
	int keyTypeMode = 0;
	UCHAR keyType[] = ZPK_TYPE;
	int index = -1;
	UCHAR *mac = NULL;
	int authenDataLen = NULL;
	UCHAR *authenData = NULL;
	UCHAR *iv = NULL;
	UCHAR *cv = NULL;

	__try {
		ret = exportrsadeskey(
			comid,
			msghdlen,
			msghd,
			algo,
			sig_alg,
			pad_mode,
			mgfHash,
			OAEP_parm_len,
			OAEP_parm,
			keyBlockType,
			keyBlockTemplateLen,
			keyBlockTemplate,
			keyOffset,
			chkLen,
			chkOffset,
			keyLen,
			keyTypeMode,
			keyType,
			wkLmk,
			index,
			mac,
			public_key,
			public_key_len,
			authenDataLen,
			authenData,
			iv,
			cv,
			data,
			data_length
		);
		if (ret < 0) {
			return ret;
		}
	}
	__finally
	{
		CloseHsmDevice(comid);
	}
	return 0;
}

int importrsadeskeyImpl(UCHAR * data, int data_length, UCHAR * private_key, int  private_key_len, UCHAR * wkLmk, int * keylen, UCHAR * cv) {
	int timeout = 0;
	int comid;
	int ret;

	comid = InitHsmDevice(getHsmIP(),getHsmPORT(),timeout);
	if (comid < 0) {
		return comid;
	}
	//////////////////////////
	int msghdlen = 0;
	char * msghd = NULL;
	int algo = 0;
	int sig_alg = 01;
	int pad_mode = 04;
	int mgfHash = NULL;
	int OAEP_parm_len = NULL;
	UCHAR *OAEP_parm = NULL;
	int keyBlockType = 3;
	int keyBlockTemplateLen = NULL;
	UCHAR *keyBlockTemplate = NULL;
	int keyOffset = NULL;
	int chkLen = NULL;
	int chkOffset = NULL;
	int keyTypeMode = 0;
	UCHAR keyType[] = ZPK_TYPE;
	int index = 99;
	UCHAR *mac = NULL;
	int authenDataLen = NULL;
	UCHAR *authenData = NULL;
	UCHAR *iv = NULL;
	CHAR lmkSchem = 'X';
	CHAR cvFlag = 0;

	__try {
		ret = importrsadeskey(
			comid,
			msghdlen, msghd,
			algo,
			sig_alg,
			pad_mode,
			mgfHash,
			OAEP_parm_len,
			OAEP_parm,
			keyBlockType,
			keyBlockTemplateLen,
			keyBlockTemplate,
			keyOffset,
			chkLen,
			chkOffset,
			keyTypeMode,
			keyType,
			data_length,
			data,
			index,
			private_key_len,
			private_key,
			lmkSchem,
			cvFlag,
			iv,
			cv,
			wkLmk,
			keylen
		);
		if (ret < 0) {
			return ret;
		}
	}
	__finally
	{
		CloseHsmDevice(comid);
	}
	return 0;
}

int encryptDecryptImpl() {
	char ip[] = "192.168.1.205";
	int port = 8000;
	int timeout = 0;
	int comid;
	int ret;

	comid = InitHsmDevice(ip, port, timeout);
	if (comid < 0) {
		puts("connect error");
	}
	puts("------------>connect success");
	/////////////////////////////////////
	/////////////////////////////////////
	__try {
		int algo = 0;
		int dataBlockFlag = 0;
		int encryptFlag = 0;
		int algoOperationMode = 0;
		int inputFormat = 1;
		int outputFormat = 1;
		char keyType[] = ZEK_TYPE;
		char key[] = "XCB8B629920622806464AB4C738053BFA";
		int paddingMode = 0;
		char paddingChar[] = "0000";
		int paddingFlag = 0;
		char *iv = NULL;
		int outFlag;
		int dataLen = 8;
		char data[] = "AAAAAAAAAAAAAAAA";

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
			printf("RET:%d\n", ret);
			return ret;
		}
		printf("LEN:%d\n", dataLen);
		printf("DATA:%s\n", data);

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
			printf("RET:%d\n", ret);
			return ret;
		}
		printf("LEN:%d\n", dataLen);
		printf("DATA:%s\n", data);
	}
	__finally
	{
		CloseHsmDevice(comid);
	}
	return 0;
}

int genhashImpl() {
	char ip[] = "192.168.1.209";
	int port = 8000;
	int timeout = 0;
	int ret = -1;
	int cmdid;

	cmdid = InitHsmDevice(ip, port, timeout);
	if (cmdid<0) {
		puts("connect error");
	}
	puts("connect success");

	ret = testHSM(cmdid, 0, NULL, NULL, NULL);
	if (ret != 0) {
		printf("RETURN: %d\n", ret);
	}

	const int rndLen = 64;
	UCHAR rnd[rndLen + 1];

	ret = genrandom(cmdid, 0, NULL, rndLen, rnd);
	if (ret != 0) {
		printf("RETURN: %d LINE: %d\n", ret, __LINE__);
	}
	puts("------------------------RANDOM------------------------------");
	for (int i = 0; i<rndLen; i++) {
		printf("%02X ", rnd[i]);
	}
	printf("\n");

	int hash_id = HASH_SHA1;
	UCHAR data[] = "zhuheng";
	int data_len = strlen((char*)data);
	UCHAR hash_value[256];
	memset(hash_value, 0x00, sizeof(hash_value));

	ret = genhash(cmdid, 0, NULL, hash_id, data_len, data, hash_value);
	if (ret != 0) {
		printf("RETURN: %d LINE: %d\n", ret, __LINE__);
	}

	printf("DATA LEN: %d\n", data_len);
	puts("------------------------HASH------------------------------");
	for (int i = 0; i < (int)strlen((char*)hash_value); i++) {
		printf("%02X ", hash_value[i]);
	}
	printf("\n");

	CloseHsmDevice(cmdid);
	return ret;
}

int rsaprisignImpl() {

	UCHAR public_key[4096];
	int  public_key_len = sizeof(public_key);
	UCHAR private_key[4096];
	int  private_key_len = sizeof(private_key);
	//testEight(public_key, &public_key_len, private_key, &private_key_len);
	//printf("PUBLEN: %d PRILEN:%d\n", public_key_len, private_key_len);

	puts("------------------------------------------------------------------------");

	////////////////////////////////
	char ip[] = "192.168.1.205";
	int port = 8000;
	int timeout = 0;
	int comid;
	int ret;

	comid = InitHsmDevice(ip, port, timeout);
	if (comid < 0) {
		puts("connect error");
	}
	puts("------------>connect success");
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
		int data_length = 8;
		UCHAR data[] = "zhuheng0";
		int index = 99;
		UCHAR sign[4096];
		int sign_length;
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
			private_key_len,
			private_key,
			sign,
			&sign_length);
		if (ret < 0) {
			printf("RET:%d\n", ret);
			return ret;
		}
		printf("LEN:%d\n", sign_length);

		///////////////////////////////////

		ret = rsapubverify(comid, msghdlen, msghd, hash_id, sign_id,
			pad_mode,
			mgfHash,
			OAEP_parm_len,
			OAEP_parm,
			pssRule,
			trailerField,
			sign_length,
			sign,
			data_length,
			data,
			index,
			NULL,
			public_key,
			public_key_len,
			authenDataLen,
			authenData
		);
		printf("RET:%d\n", ret);
	}
	__finally
	{
		CloseHsmDevice(comid);
	}
	////////////////////////////////
	return 0;
}

int rsapubverifyImpl() {

	UCHAR public_key[4096];
	int  public_key_len = sizeof(public_key);
	UCHAR private_key[4096];
	int  private_key_len = sizeof(private_key);
	//testEight(public_key, &public_key_len, private_key, &private_key_len);
	//printf("PUBLEN: %d PRILEN:%d\n", public_key_len, private_key_len);

	puts("------------------------------------------------------------------------");

	////////////////////////////////
	char ip[] = "192.168.1.205";
	int port = 8000;
	int timeout = 0;
	int comid;
	int ret;

	comid = InitHsmDevice(ip, port, timeout);
	if (comid < 0) {
		puts("connect error");
	}
	puts("------------>connect success");
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
		int data_length = 8;
		UCHAR data[] = "zhuheng0";
		int index = 99;
		UCHAR sign[4096];
		int sign_length;
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
			private_key_len,
			private_key,
			sign,
			&sign_length);
		if (ret<0) {
			printf("RET:%d\n", ret);
			return ret;
		}
		printf("LEN:%d\n", sign_length);

		///////////////////////////////////

		ret = rsapubverify(comid, msghdlen, msghd, hash_id, sign_id,
			pad_mode,
			mgfHash,
			OAEP_parm_len,
			OAEP_parm,
			pssRule,
			trailerField,
			sign_length,
			sign,
			data_length,
			data,
			index,
			NULL,
			public_key,
			public_key_len,
			authenDataLen,
			authenData
		);
		printf("RET:%d\n", ret);
	}
	__finally
	{
		CloseHsmDevice(comid);
	}
	return ret;
	////////////////////////////////
}

int generateKeyImpl(int comid, PHKEY_Z  hKey,int algo) {
	char key[255];
	char checkValue[6 + 1];
	int genMod;
	genMod = 0;
	int ret = 0;
	char * keyType = ZEK_TYPE;

	ret = generateKey(comid, 0, NULL, algo, genMod, keyType, 'X', key, checkValue);
	if (ret<0) {
		VarLogEntry(" CPGenKey", "DES/TDES error", ret, 0);
		return ret;
	}
	hKey = (HKEY_Z*)malloc(sizeof(HKEY_Z));
	if (NULL == hKey) {
		VarLogEntry(" CPGenKey", "memory error", -1, 0);
		return -1;
	}
	hKey->len = strlen(key);
	memcpy(hKey->key, key, hKey->len);
	memcpy(hKey->cv, checkValue, strlen(checkValue));
	sprintf((CHAR*)hKey->ALGID, "%02d",algo);
	sprintf((CHAR*)hKey->KEYLEN,"%04d",hKey->len);
	memcpy(hKey->keyType, keyType,strlen(keyType));
	return ret;
}

//判断是否初始化
int initJudgment(HCRYPTPROV hProv) {
	//容器是否初始化
	if (!(getMutexFlag() & hProv)) {
		VarLogEntry(" HCRYPTPROV hProv", "error %d %u", -1, 0, getMutexFlag(), hProv);
		return -1;
	}
	return 0;
}

int getKeyParamImpl(CHAR * data, LPBYTE pbData, LPDWORD pcbDataLen) {
	int ret = 0;

	*pcbDataLen = strlen(data);
	if (pcbDataLen <= 0) {
		return  -1;
	}

	if (NULL != pbData) {
		memcpy(pbData, data, *pcbDataLen);
	}

	return ret;
}

int getKeyParam(DWORD dwParam, HKEY_Z * tmpKey, LPBYTE pbData, LPDWORD pcbDataLen) {
	int ret = 0;

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
		ret = getKeyParamImpl((CHAR*)tmpKey->ALGID,  pbData,  pcbDataLen);
		break;
	case KP_BLOCKLEN:
		ret = getKeyParamImpl((CHAR*)tmpKey->BLOCKLEN, pbData, pcbDataLen);
		break;
	case KP_KEYLEN:
		ret = getKeyParamImpl((CHAR*)tmpKey->KEYLEN, pbData, pcbDataLen);
		break;
	case KP_SALT:
		ret = getKeyParamImpl((CHAR*)tmpKey->SALT, pbData, pcbDataLen);
		break;
	case KP_PERMISSIONS:
		ret = getKeyParamImpl((CHAR*)tmpKey->PERMISSIONS, pbData, pcbDataLen);
		break;
	case KP_IV:
		ret = getKeyParamImpl((CHAR*)tmpKey->IV, pbData, pcbDataLen);
		break;
	case KP_PADDING:
		ret = getKeyParamImpl((CHAR*)tmpKey->PADDING, pbData, pcbDataLen);
		break;
	case KP_MODE:
		ret = getKeyParamImpl((CHAR*)tmpKey->MODE, pbData, pcbDataLen);
		break;
	case KP_MODE_BITS:
		ret = getKeyParamImpl((CHAR*)tmpKey->MODE_BITS, pbData, pcbDataLen);
		break;
	case KP_EFFECTIVE_KEYLEN:
		ret = getKeyParamImpl((CHAR*)tmpKey->EFFECTIVE_KEYLEN, pbData, pcbDataLen);
		break;
	default:
		VarLogEntry(" CPGetKeyParam", "dwParam error", dwParam, 0);
		return -1;
	}

	if (0 != ret ) {
		VarLogEntry(" CPGetKeyParam", "dwParam Empty error", dwParam, 0);
	}
	return ret;
}

int getHashParam(DWORD dwParam, PHHASH_Z phzHash , LPBYTE pbData, LPDWORD pdwDataLen) {
	int ret = 0;
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
		ret = getKeyParamImpl((CHAR*)phzHash->ALGID, pbData, pdwDataLen);
		break;
	case HP_HASHVAL:
		ret = getKeyParamImpl((CHAR*)phzHash->HASHVAL, pbData, pdwDataLen);
		break;
	case HP_HASHSIZE:
		ret = getKeyParamImpl((CHAR*)phzHash->HASHSIZE, pbData, pdwDataLen);
		break;
	case HP_HMAC_INFO:
		ret = getKeyParamImpl((CHAR*)phzHash->HMAC_INFO, pbData, pdwDataLen);
		break;
	case HP_TLS1PRF_LABEL:
		ret = getKeyParamImpl((CHAR*)phzHash->TLS1PRF_LABEL, pbData, pdwDataLen);
		break;
	case HP_TLS1PRF_SEED:
		ret = getKeyParamImpl((CHAR*)phzHash->TLS1PRF_SEED, pbData, pdwDataLen);
		break;
	default:
		VarLogEntry(" CPGetHashParamImpl", "dwParam error", dwParam, 0);
		return (-1);
	}
	if (0 != ret) {
		VarLogEntry(" getHashParam", "dwParam Empty error", dwParam, 0);
	}
	return ret;
}

int genSm2Key_backup() {
	int ret = 0;

	///////////////////////////
	int comid;
	int msghdlen = 0;
	char *msghd = NULL;
	int algflag = 3;
	int key_usage = 3;
	int compflag = 4;
	int key_length = 256;
	int index = 01;
	int Plen = 0;
	UCHAR *Pbuf = NULL;
	int Alen = 0;
	UCHAR *Abuf = NULL;
	int Blen = 0;
	UCHAR *Bbuf = NULL;
	int Gxlen = 0;
	UCHAR *Gxbuf = NULL;
	int Gylen = 0;
	UCHAR *Gybuf = NULL;
	int Nlen = 0;
	UCHAR *Nbuf = NULL;
	UCHAR puKey[4096], pvKey[4096], deKey[4096];
	int puLen, pvLen, deLen;
	UCHAR *public_key = puKey;
	int * public_key_len = &puLen;
	UCHAR *private_key = pvKey;;
	int *private_key_len = &pvLen;
	UCHAR *derpubkey = deKey;
	int * derpubkeylen = &deLen;
	///////////////////////////////////////////

	char ip[] = "192.168.1.205";
	int port = 8000;
	int timeout = 0;

	comid = InitHsmDevice(ip, port, timeout);
	if (comid < 0) {
		puts("connect error");
	}
	puts("------------>connect success");
	__try {
		ret = gensm2key(comid, msghdlen, msghd, algflag, key_usage, compflag, key_length, index, Plen, Pbuf, Alen, Abuf, Blen,
			Bbuf, Gxlen, Gxbuf, Gylen, Gybuf, Nlen, Nbuf, public_key, public_key_len, private_key, private_key_len, derpubkey, derpubkeylen);
		printf("RET:%d\n", ret);
		printf("PUBLIC KEY LEN %d\n", puLen);
		printf("PUBLIC KEY LEN %d\n", pvLen);
		printf("PUBLIC KEY LEN %d\n", deLen);
	}
	__finally {
		CloseHsmDevice(comid);
	}
	return ret;
}

int genSm2Key(PHKEY_Z hKey,int comid, int Algid) {
	int ret = 0;

	///////////////////////////
	int msghdlen = 0;
	char *msghd = NULL;
	int algflag = 3;
	int key_usage = 3;
	int compflag = 4;
	int key_length = 256;
	int index = 01;
	int Plen = 0;
	UCHAR *Pbuf = NULL;
	int Alen = 0;
	UCHAR *Abuf = NULL;
	int Blen = 0;
	UCHAR *Bbuf = NULL;
	int Gxlen = 0;
	UCHAR *Gxbuf = NULL;
	int Gylen = 0;
	UCHAR *Gybuf = NULL;
	int Nlen = 0;
	UCHAR *Nbuf = NULL;
	UCHAR puKey[4096], pvKey[4096], deKey[4096];
	int puLen, pvLen, deLen;

	///////////////////////////////////////////

	char * ip = getHsmIP();
	int port = getHsmPORT();
	int timeout = 0;

	ret = gensm2key(comid, msghdlen, msghd, algflag, key_usage, compflag, key_length, index, Plen, Pbuf, Alen, Abuf, Blen,
		Bbuf, Gxlen, Gxbuf, Gylen, Gybuf, Nlen, Nbuf, puKey, &puLen, pvKey, &pvLen, deKey, &deLen);
	if (ret != 0) {
		VarLogEntry(" genSm2Key", "gensm2key error", ret, 0);
		return -1;
	}
	hKey = (PHKEY_Z)malloc(sizeof(HKEY_Z));
	if (NULL == hKey) {
		VarLogEntry(" genSm2Key", "Memory error", ret, 0);
		return -1;
	}
	sprintf((CHAR*)hKey->ALGID,"%02d",Algid);
	memcpy(puKey, hKey->puKey , puLen);
	memcpy(pvKey, hKey->pvKey, pvLen);
	memcpy(deKey, hKey->derPuKey , deLen);
	hKey->puLen = puLen;
	hKey->pvLen = pvLen;
	hKey->derPuLen =  deLen;
	return ret;
}

int decryptAlgo(int comid, PHKEY_Z phKey, BYTE *pbData, DWORD *pdwDataLen) {
	int ret = 0;
	int algo;

	algo = atoi((CHAR*)phKey->ALGID);
	switch (algo)
	{
	case ALGO_DESTDES:
		ret = decryptDES( comid,  phKey, pbData, pdwDataLen);
		break;
	case ALGO_RSA:
		ret = decryptRSA(comid, phKey, pbData, pdwDataLen);
		break;
	case ALGO_SM2:
		ret = decryptSM2(comid, phKey, pbData, pdwDataLen);
		break;
	default:

		return -1;
	}
	return ret;
}

int encryptAlgo(int comid, PHKEY_Z phKey, BYTE *pbData, DWORD *pdwDataLen) {
	int ret = 0;
	int algo;

	algo = atoi((CHAR*)phKey->ALGID);
	switch (algo)
	{
	case ALGO_DESTDES:
		ret = encryptDES(comid, phKey, pbData, pdwDataLen);
		break;
	case ALGO_RSA:
		ret = encryptRSA(comid, phKey, pbData, pdwDataLen);
		break;
	case ALGO_SM2:
		ret = encryptSM2(comid, phKey, pbData, pdwDataLen);
		break;
	default:
		VarLogEntry(" encryptAlgo", "key algo error", algo, 0);
	}

	return ret;
}

int decryptRSA(int comid, PHKEY_Z phKey, BYTE *pbData, DWORD *pdwDataLen) {
	int ret = 0;

	return ret;
}

int encryptRSA(int comid, PHKEY_Z phKey, BYTE *pbData, DWORD *pdwDataLen) {
	int ret = 0;

	return ret;
}

int decryptSM2(int comid, PHKEY_Z phKey, BYTE *pbData, DWORD *pdwDataLen) {
	int ret = 0;

	return ret;
}

int encryptSM2(int comid, PHKEY_Z phKey, BYTE *pbData, DWORD *pdwDataLen) {
	int ret = 0;

	return ret;
}

int decryptDES(int comid, PHKEY_Z phKey, BYTE *pbData, DWORD *pdwDataLen) {
	int ret = 0;
	int algo = 0;
	int dataBlockFlag = 0;
	int encryptFlag = 0;
	int algoOperationMode = 0;
	int inputFormat = 1;
	int outputFormat = 1;
	char * keyType = (CHAR*)phKey->keyType;
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
		(CHAR*)phKey->key,
		paddingMode,
		paddingChar,
		paddingFlag,
		iv,
		&outFlag,
		(int *)pdwDataLen,
		(CHAR*)pbData);
	if (ret < 0) {
		VarLogEntry(" CPDecryptImpl", "encryptDecrypt error", ret, 0);
	}
	return ret;
}

int encryptDES(int comid, PHKEY_Z phKey, BYTE *pbData, DWORD *pdwDataLen) {
	int ret;
	int algo = 0;
	int dataBlockFlag = 0;
	int encryptFlag = 0;
	int algoOperationMode = 0;
	int inputFormat = 1;
	int outputFormat = 1;
	char * keyType = (CHAR*)phKey->keyType;
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
		(CHAR*)phKey->key,
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
	return ret;
}


