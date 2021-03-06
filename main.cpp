#include <iostream>
#include <io.h>
#include <cstring>
#include "MVIDCodeReader.h"
#include "process.h"
#include <windows.h>
#include <atlstr.h>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <atltime.h>

//using namespace std;
using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

class Reader {
private:
    bool m_bProcess = true;
    void *m_handle{};
    MVID_CAM_OUTPUT_INFO *m_pstOutput{};
    wchar_t *billCode;
    char *new_code;
    char old_code[MVID_MAX_CODECHARATERLEN];
    MVID_IMAGE_INFO pstOutputImage = {0};
    char filename[256] = {0};
    char filenameKey[256] = {0};
    CTime currTime;
public:
    void Process();

    bool initDevices();

    void send();

    void saveImage(MVID_IMAGE_INFO pstInputImage);
};

static unsigned int __stdcall ProcessThread(void *pUser) {
    auto *pThis = (Reader *) pUser;
    if (pThis) {
        pThis->Process();
    }
    return 0;
}

bool Char2Wchar(const char *pStr, wchar_t *pOutWStr, int nOutStrSize) {
    if (!pStr || !pOutWStr) {
        return false;
    }

    UINT nChgType = CP_UTF8;

    int iLen = MultiByteToWideChar(nChgType, 0, (LPCSTR) pStr, -1, NULL, 0);

    memset(pOutWStr, 0, sizeof(wchar_t) * nOutStrSize);

    if (iLen >= nOutStrSize) {
        iLen = nOutStrSize - 1;
    }

    MultiByteToWideChar(nChgType, 0, (LPCSTR) pStr, -1, pOutWStr, iLen);

    pOutWStr[iLen] = 0;

    return true;
}

void Reader::Process() {
    int nRet = MVID_CR_OK;
    long j = 0;
    m_pstOutput = (MVID_CAM_OUTPUT_INFO *) malloc(sizeof(MVID_CAM_OUTPUT_INFO));

    memset(m_pstOutput, 0, sizeof(MVID_CAM_OUTPUT_INFO));
    while (m_bProcess) {
        std::cout << "read" << j++ << std::endl;
        nRet = MVID_CR_CAM_GetOneFrameTimeout(m_handle, m_pstOutput, 100);

        if (MVID_CR_OK == nRet) {
            //ch:输出结果 | en:Output results
//            strMsg.Format(_T("已识别 %d 个对象："), m_pstOutput->stCodeList.nCodeNum);
            for (int i = 0; i < m_pstOutput->stCodeList.nCodeNum; ++i) {
                if (strcmp((char *)m_pstOutput->stCodeList.stCodeInfo[i].strCode, "yjcx.chinapost.com.cn") == 0) {
                    continue;
                }
                wchar_t strWchar[MVID_MAX_CODECHARATERLEN] = {0};
                Char2Wchar((char *) m_pstOutput->stCodeList.stCodeInfo[i].strCode, strWchar, MVID_MAX_CODECHARATERLEN);
                std::wstring ws(strWchar);
                std::string str(ws.begin(), ws.end());
                billCode = strWchar;
                new_code = (char *) m_pstOutput->stCodeList.stCodeInfo[i].strCode;


//                输出条码信息
                std::cout << "new_code: " << new_code << std::endl;
                std::cout << "old_code: " << old_code << std::endl;
                if (strcmp(new_code, old_code) != 0) {
                    strcpy(old_code, new_code);

                    currTime = CTime::GetCurrentTime();

                    sprintf(filenameKey, ("%s_%.4d%.2d%.2d%.2d%.2d%.2d"), new_code, currTime.GetYear(),
                            currTime.GetMonth(),
                            currTime.GetDay(), currTime.GetHour(), currTime.GetMinute(), currTime.GetSecond());
                    send();
                    saveImage(m_pstOutput->stImage);
                }
            }
        } else {
//            strMsg.Format(_T("识别失败，错误码 %x"), nRet);
        }

    }
}

bool Reader::initDevices() {
    // ch:初始化设备信息列表 | en:Device Information List Initialization
    MVID_CAMERA_INFO_LIST *m_pstDevList = new MVID_CAMERA_INFO_LIST();
    if (NULL == m_pstDevList) { return false; }
    memset(m_pstDevList, 0, sizeof(MVID_CAMERA_INFO_LIST));

    // ch:枚举子网内所有设备 | en:Enumerate all devices within subnet
    int nRet = MVID_CR_CAM_EnumDevices(m_pstDevList);
    if (MVID_CR_OK != nRet) { return false; }

    int nIp1, nIp2, nIp3, nIp4;
    for (int i = 0; i < m_pstDevList->nCamNum; i++) {
        MVID_CAMERA_INFO *pDeviceInfo = m_pstDevList->pstCamInfo[i];
        if (NULL == pDeviceInfo) {
            continue;
        }
        if (pDeviceInfo->nCamType == MVID_GIGE_CAM) {
            nIp1 = ((pDeviceInfo->nCurrentIp & 0xff000000) >> 24);
            nIp2 = ((pDeviceInfo->nCurrentIp & 0x00ff0000) >> 16);
            nIp3 = ((pDeviceInfo->nCurrentIp & 0x0000ff00) >> 8);
            nIp4 = (pDeviceInfo->nCurrentIp & 0x000000ff);

            std::cout << nIp1 << nIp2 << nIp3 << nIp4 << std::endl;

            int nRet = MVID_CR_CreateHandle(&m_handle, MVID_BCR | MVID_TDCR);
            if (MVID_CR_OK != nRet) {
//                ShowErrorMsg(TEXT("MVID_CR_CreateHandle "), nRet);
                return false;
            }
            nRet = MVID_CR_CAM_BindDevice(m_handle, m_pstDevList->pstCamInfo[i]);
            if (MVID_CR_OK != nRet) {
//                ShowErrorMsg(TEXT("MVID_CR_CAM_BindDevice "), nRet);
                return false;
            }
            nRet = MVID_CR_CAM_StartGrabbing(m_handle);
            if (MVID_CR_OK != nRet) {
//                ShowErrorMsg(TEXT("MVID_CR_CAM_StartGrabbing "), nRet);
                return false;
            }
            unsigned int nThreadID = 0;
            HANDLE *m_hThread = (HANDLE *) _beginthreadex(NULL, 0, ProcessThread, this, 0, &nThreadID);
            if (NULL == m_hThread) {
//                ShowErrorMsg(TEXT("_beginthreadex fail"), 0);
                return false;
            }
        }
    }
    return true;
}

void Reader::send() {
    wchar_t strWchar[MVID_MAX_CODECHARATERLEN] = {0};
    Char2Wchar(filenameKey, strWchar, MVID_MAX_CODECHARATERLEN);
    std::wstring ws(strWchar);
    std::string str(ws.begin(), ws.end());


    if (!strWchar) {
        return;
    }
    auto fileStream = std::make_shared<ostream>();

    // Open stream to output file.
    pplx::task<void> requestTask = fstream::open_ostream(U("results.html")).then([=](ostream outFile) {
                *fileStream = outFile;

                // Create http_client to send the request.
                http_client client(U("http://127.0.0.1:8900/"));

                // Build request URI and start the request.
                uri_builder builder(U("/search"));
                builder.append_query(U("billCode"), strWchar);
                return client.request(methods::GET, builder.to_string());
            })

                    // Handle response headers arriving.
            .then([=](http_response response) {
                printf("Received response status code:%u\n", response.status_code());

                // Write response body into the file.
                return response.body().read_to_end(fileStream->streambuf());
            })

                    // Close the file stream.
            .then([=](size_t) {
                return fileStream->close();
            });

    // Wait for all the outstanding I/O to complete and handle any exceptions
    try {
        requestTask.wait();
    }
    catch (const std::exception &e) {
        printf("Error exception:%s\n", e.what());
    }
}

void Reader::saveImage(MVID_IMAGE_INFO pstInputImage) {
    int nRet = MVID_CR_OK;
    nRet = MVID_CR_SaveImage(m_handle, &m_pstOutput->stImage, MVID_IMAGE_JPEG, &pstOutputImage, 80);
    if (MVID_CR_OK == nRet) {
        // ch:保存图像 | en:Save image
        // ch:获取系统时间作为保存图片文件名 | en:Get the system time as the name of saved picture file

        sprintf(filename, ("D:\\scan\\%s.jpg"), filenameKey);
        FILE *pfile = fopen(filename, "wb");
        if (pfile == NULL) {
            return;
        }
        fwrite(pstOutputImage.pImageBuf, 1, pstOutputImage.nImageLen, pfile);
        fclose(pfile);
        pfile = NULL;
    }
}

int main() {
    Reader *pMain = new Reader();
    pMain->initDevices();
    std::cout << "Bye, World!" << std::endl;
    long i = 1 << 30;
    std::cout << "start sleep, " << i << std::endl;
    Sleep(i);
    return 1;
}
