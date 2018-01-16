// ConsoleApplication1.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <tchar.h>

/*
int Create() {
	HKEY hKey;
	LPCTSTR lpRun = _T("SoftWare\\_MyTest");
	DWORD state, dwType, sizeBuff;
	long lRet;
	WCHAR reBuff[14] = {0};

	//创建子键
	//lRet = RegCreateKeyEx(HKEY_CURRENT_USER,lpRun,0,NULL,0,0,NULL,&hKey,&state);
	lRet = RegCreateKeyEx(HKEY_CURRENT_USER, lpRun, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &state);
	if (lRet == ERROR_SUCCESS) {
		if (state == REG_CREATED_NEW_KEY) {
			puts("表项创建成功");
		}

		RegCloseKey(hKey);
	}

	//打开 修改键值
	lRet = RegOpenKeyEx(HKEY_CURRENT_USER,lpRun,0,KEY_WRITE,&hKey);
	if (lRet == ERROR_SUCCESS) {
		RegSetValueEx(hKey,_T("test"),0,REG_SZ,(BYTE*)_T("success"),14);
		puts("打开 修改键值");
		RegCloseKey(hKey);
	}

	//读取键值
	lRet = RegOpenKeyEx(HKEY_CURRENT_USER,lpRun,0,KEY_READ,&hKey);
	if (lRet == ERROR_SUCCESS) {
		sizeBuff = sizeof(reBuff);
		lRet = RegQueryValueEx(hKey,_T("test"),0,&dwType,(BYTE*)reBuff,&sizeBuff);
		if (lRet == ERROR_SUCCESS) {
			puts("SUCCESS");
			wprintf(_T("BUFF: %s\n"),reBuff);
		}
		else {
			printf("ERROR: %ul",lRet);
			RegCloseKey(hKey);
		}
	}

		删除键
	lRet = RegOpenKeyEx(HKEY_CURRENT_USER,lpRun,0,KEY_WRITE,&hKey);
	if (lRet == ERROR_SUCCESS) {
		//删除
		RegDeleteValue(hKey,_T("test"));

		puts("删除键成功");
		RegCloseKey(hKey);
		system("pause");
	}
	return lRet;
}


void Create2() {
	long lRet = 1L;
	HKEY hKey;

	lRet = GMN_RegOpen(&hKey);
	printf("REV: %u\n", lRet);


	char csKey1[] = "CSPname";
	char csValue1[] = "北京江南哥盟科技有限公司";
	DWORD len1 = strlen(csValue1);

	char csKey2[] = "Tpye";
	DWORD csValue2 = 1;
	DWORD len2 = sizeof(DWORD);

	char csKey3[] = "SigInFile";
	DWORD csValue3 = 0;
	DWORD len3 = sizeof(DWORD);

	char csKey4[] = "Signature";
	char csValue4[] = "Signature";
	DWORD len4 = strlen(csValue4);

	char csKey5[] = "ImagePath";
	char csValue5[] = "ImagePath";
	DWORD len5 = strlen(csValue5);

	lRet = GMN_RegSetValueEx(hKey, csKey1, 0, REG_SZ, (BYTE*)csValue1, len1);
	printf("REV: %u\n", lRet);

	lRet = GMN_RegSetValueEx(hKey, csKey2, 0, REG_DWORD, (LPBYTE)&csValue2, len2);
	printf("REV: %u\n", lRet);

	lRet = GMN_RegSetValueEx(hKey, csKey3, 0, REG_DWORD, (LPBYTE)&csValue3, len3);
	printf("REV: %u\n", lRet);

	lRet = GMN_RegSetValueEx(hKey, csKey4, 0, REG_BINARY, (BYTE*)csValue4, len4);
	printf("REV: %u\n", lRet);

	lRet = GMN_RegSetValueEx(hKey, csKey5, 0, REG_SZ, (BYTE*)csValue5, len5);
	printf("REV: %u\n", lRet);


	char csValue6[1024];
	DWORD sizeBuff1 = 1024;
	DWORD dwType1;

	char csValue7[1024];
	DWORD sizeBuff2 = 1024;
	DWORD dwType2;

	char csValue8[1024];
	DWORD sizeBuff3 = 1024;
	DWORD dwType3;

	char csValue9[1024];
	DWORD sizeBuff4 = 1024;
	DWORD dwType4;

	char csValue10[1024];
	DWORD sizeBuff5 = 1024;
	DWORD dwType5;

	lRet = GMN_RegQueryValueEx(hKey, csKey1, 0, &dwType1, (BYTE*)csValue6, &sizeBuff1);
	printf("REV: %u | key: %s | len: %u | value: %s | type: %u \n", lRet,
		csKey1,
		sizeBuff1,
		csValue6,
		dwType1);

	lRet = GMN_RegQueryValueEx(hKey, csKey2, 0, &dwType2, (BYTE*)csValue7, &sizeBuff2);
	printf("REV: %u | key: %s | len: %u | value: %s | type: %u \n", lRet,
		csKey2,
		sizeBuff2,
		csValue7,
		dwType2);

	lRet = GMN_RegQueryValueEx(hKey, csKey3, 0, &dwType3, (BYTE*)csValue8, &sizeBuff3);
	printf("REV: %u | key: %s | len: %u | value: %s | type: %u \n", lRet,
		csKey3,
		sizeBuff3,
		csValue8,
		dwType3);

	lRet = GMN_RegQueryValueEx(hKey, csKey4, 0, &dwType4, (BYTE*)csValue9, &sizeBuff4);
	csValue9[sizeBuff4] = '\0';
	printf("REV: %u | key: %s | len: %u | value: %s | type: %u \n", lRet,
		csKey4,
		sizeBuff4,
		csValue9,
		dwType4);

	lRet = GMN_RegQueryValueEx(hKey, csKey5, 0, &dwType5, (BYTE*)csValue10, &sizeBuff5);
	printf("REV: %u | key: %s | len: %u | value: %s | type: %u \n", lRet,
		csKey5,
		sizeBuff5,
		csValue10,
		dwType5);



	GMN_RegDeleteValue(hKey, csKey1);
	GMN_RegDeleteValue(hKey, csKey2);
	GMN_RegDeleteValue(hKey, csKey3);
	GMN_RegDeleteValue(hKey, csKey4);
	GMN_RegDeleteValue(hKey, csKey5);


	GMN_RegCloseKey(hKey);
}

void testOne() {
	char test[] = "zhuheng";
	DWORD dwTest = (DWORD)test;
	printf("S: %s\n", (CHAR *)dwTest);

	printf("%d\n", strlen((char*)test));
	printf("%d\n", strlen((char*)dwTest));
}

void testTwo() {
	unsigned 
	char test[] = "zhuheng";
	BYTE *bs = test;
	printf("%s\n",test);
	printf("%s\n", bs);
	
	
	unsigned char ucs[] = "12345678";

	printf("%d %d\n",sizeof(test),sizeof(bs));
	printf("%d %d\n", strlen((char*)test) , strlen((char*)bs));

}

void testThree() {
	HKEY hKey;
	LONG lRet;
	DWORD dwFlag;
	BYTE pbData[1024];
	DWORD pdwDataLen;

	lRet = GMN_RegOpen(&hKey);
	printf("REV: %u\n", lRet);

	lRet = GMN_RegQueryValueEx(hKey,"zhuheng001" , 0,
		&dwFlag, pbData, &pdwDataLen);
	printf("REV: %u\n", lRet);
	printf("len: %u buff:%s type: %u",pdwDataLen,pbData,dwFlag);
}
*/

void testFour() {
	char ip[] = "192.168.1.209";
	int port = 8000;
	int timeout = 0;
	int ret = -1;
	int cmdid;

	cmdid = InitHsmDevice(ip,port,timeout);
	if (cmdid<0) {
		puts("connect error");
	}
	puts("connect success");
	
	ret = testHSM(cmdid,0,NULL,NULL,NULL);
	if (ret!=0) {
		printf("RETURN: %d\n",ret);
	}

	const int rndLen = 64;
	UCHAR rnd[rndLen + 1];

	ret = genrandom(cmdid,0,NULL,rndLen,rnd);
	if (ret != 0) {
		printf("RETURN: %d LINE: %d\n", ret,__LINE__);
	}
	puts("------------------------RANDOM------------------------------");
	for (int i=0; i<rndLen;i++) {
		printf("%02X ",rnd[i]);
	}
	printf("\n");
	
	int hash_id = HASH_SHA1;
	UCHAR data[] = "zhuheng";
	int data_len = strlen((char*)data);
	UCHAR hash_value[256];
	memset(hash_value,0x00,sizeof(hash_value));

	ret = genhash(cmdid, 0, NULL, hash_id, data_len, data, hash_value);
	if (ret != 0) {
		printf("RETURN: %d LINE: %d\n", ret, __LINE__);
	}

	printf("DATA LEN: %d\n",data_len);
	puts("------------------------HASH------------------------------");
	for (int i = 0; i<strlen((char*)hash_value); i++) {
		printf("%02X ", hash_value[i]);
	}
	printf("\n");

	CloseHsmDevice(cmdid);

}

void testFive() {
	char ip[] = "192.168.1.209";
	int port = 8000;
	int timeout = 0;
	int comid;
	int ret;

	comid = InitHsmDevice(ip, port, timeout);
	if (comid < 0) {
		puts("connect error");
	}
	puts("------------>connect success");

	int rndLen = 32;
	UCHAR rnd[32 + 1];
	__try {
		ret = genrandom(comid, 0, NULL, rndLen, rnd);
		if (ret<0) {
			printf("ret:%d\n",ret);
			return;
		}
		return;
	}
	__finally
	{
		puts("--------->execute success");
		printf("rnd : %d\n",&rnd);
		for (int i=0;i<rndLen;i++) {
			printf("%02X",rnd[i]);
		}
		puts("");

		CloseHsmDevice(comid);
	}

}

int main()
{
	puts("--------------------REGEDIT TEST ------------------------>");
	//Create();
	//Create2();
	//testOne();
	//testTwo();
	//testThree();
	//testFour();
	testFive();

	getchar();
    return 0;
}



