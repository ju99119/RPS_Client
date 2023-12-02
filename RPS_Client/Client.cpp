#define _WINSOCK_DEPRECATED_NO_WARNINGS // 최신 VC++ 컴파일 시 경고 방지
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    512
#define GAWI 0
#define BAWEE 1
#define BO 2

// 소켓 함수 오류 출력 후 종료
void err_quit(char* msg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
    LocalFree(lpMsgBuf);
    exit(1);
}

// 소켓 함수 오류 출력
void err_display(char* msg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    printf("[%s] %s", msg, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
}

// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char* buf, int len, int flags)
{
    int received;
    char* ptr = buf;
    int left = len;

    while (left > 0) {
        received = recv(s, ptr, left, flags);
        if (received == SOCKET_ERROR)
            return SOCKET_ERROR;
        else if (received == 0)
            break;
        left -= received;
        ptr += received;
    }

    return (len - left);
}

// 서버와 클라이언트 간에 주고 받는 패킷 구조체 정의
#pragma pack(1)
typedef struct {
    int rps = 0; // 클라이언트가 선택한 가위바위보에 대한 플래그이다.        0 : 가위(찌), 1 : 바위(묵), 2 : 보(빠)
    int status = 0; // 처음 게임 시작 할 때 데이터를 받는 필드.              0 : 게임 시작 대기 상태, 1 : 게임 시작!
    int client_win_flag = 0; // 클라이언트가 승리할 때마다 1씩 증가.         클라이언트가 몇 판 이겼는지에 대한 Flag
    int setnum_flag = 0; // 묵찌빠 승패 판정 때마다 1씩 증가.                묵찌빠 게임을 몇 판 했는지에 대한 Flag
    int end_flag = 0; // 종료 플래그이다.                                    0 : default, 1 : 1이 들어오면 클라이언트 연결 바로 종료
    int win_flag = 0; // 가위바위보 결과 플래그.                             0 : 무승부, 1 : 클라이언트 패배(묵찌빠 방어), 2 : 클라이언트 승리(묵찌빠 공격)
    char data[64]; // 데이터 전달 필드
} RPS;
#pragma pack()

// 게임 결과를 출력해주는 함수
void return_game_rate(RPS pack) {
    if (pack.setnum_flag == 0) {
        printf("게임을 한 판도 진행하지 않아서 게임 결과가 출력되지 않습니다.\n"); // 한 판도 플레이하지 않은 경우에는 결과없음 문자열 출력
    }
    // 승률 계산하여 총 게임 수와 승리 수, 패배 수, 승률을 출력
    else {
        printf("||||||||||||||||||||||||||\n");
        printf("||\t플레이 : %d판\t||\n", pack.setnum_flag);
        printf("||\t승  리 : %d판\t||\n", pack.client_win_flag);
        int lose = (pack.setnum_flag - pack.client_win_flag);
        printf("||\t패  배 : %d판\t||\n", lose);
        float rate = ((float)pack.client_win_flag / pack.setnum_flag);
        printf("||\t승  률 : %.2f%%\t||\n", rate * 100);
        printf("||||||||||||||||||||||||||\n");
    }
}

int main(int argc, char* argv[])
{
    int retval;

    // 윈속 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // socket()
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // connect()
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("connect()");

    // 데이터 통신에 사용할 변수
    char buf[BUFSIZE + 1];
    int len;

    RPS packet; // 서버와 클라이언트 간에 주고 받는 패킷 구조체 선언

    int start_err_flag = 0; // 처음에 '시작' 입력 시에 다른 것이 입력 되었을 때의 오류처리를 위한 변수
    // 서버와 데이터 통신

    while (1) {

        while (1) {
            if (start_err_flag == 0) {
                retval = recv(sock, (char*)&packet, sizeof(packet), 0);                                      // "KJH 묵찌빠 게임" 문자열을 받아오기 위함
                if (retval == SOCKET_ERROR) {
                    err_display("recv()");
                    break;
                }
                else if (retval == 0)
                    break;
                printf("\n\t%s\n", packet.data);                                                             // "KJH 묵찌빠 게임" 출력
            }
            printf("게임을 시작하시려면 \"시작\"이라고 입력해주세요('승률확인', '종료'도 가능) : ");         // 서버에게 게임 시작 요청하기
            if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
                break;

            // '\n' 문자 제거
            len = strlen(buf);
            if (buf[len - 1] == '\n')
                buf[len - 1] = '\0';
            if (strlen(buf) == 0)
                break;

            char start_buf[5] = "시작";
            char end_buf[5] = "종료";
            char win_rate_buf[9] = "승률확인";

            // 입력받은 문자열이 '시작'이라면 status 필드를 1로 세트해서 게임 시작을 요청
            if (strcmp(buf, start_buf) == 0) {
                printf("\n\t@@@@@@@@@@@@@@@\t\t\t\t게임을 시작합니다.\t\t\t@@@@@@@@@@@@@@@");
                packet.status = 1;
                packet.win_flag = 0;
                retval = send(sock, (char*)&packet, sizeof(packet), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                }
                start_err_flag = 0;                                                                          // 게임을 반복하기 때문에, 다음 게임을 위해 error_flag 초기화
                break;
            }
            // 입력받은 문자열이 '종료'라면 end_flag를 1로 세트해서 게임 종료를 요청하고 최종 결과 출력
            else if (strcmp(buf, end_buf) == 0) {
                packet.end_flag = 1;
                send(sock, (char*)&packet, sizeof(packet), 0);
                printf("\n||||\t\t사용자의 요청으로 게임을 종료합니다. Good Bye!\t\t||||\n");
                printf("||||||||최종  승률||||||||\n");
                return_game_rate(packet);                                                                    // 게임 종료 시에 게임 결과 출력
                break;
            }
            // 입력받은 문자열이 '승률확인'이라면 승률 함수 호출
            else if (strcmp(buf, win_rate_buf) == 0) {
                return_game_rate(packet);
                start_err_flag = 1;                                                                          // 게임 시작을 위한 단계이기 때문에, 승률확인도 일종의 error로 판단하여 error_flag 1로 세트
            }
            else {
                printf("\"시작\"이라고 입력하셔야 게임이 시작됩니다. 게임을 종료하고 싶으시다면 \"종료\"를 입력해주세요.\n");
                start_err_flag = 1;                                                                          // error_flag 1로 세트하여 유저가 올바른 문자열 입력하도록 함
            }
        }

        // 게임 시작 요청 단계에서 종료를 입력했을 때, 클라이언트 소켓 종료를 위한 조건문
        if (packet.end_flag == 1)
            break;

        // 가위바위보 게임 진행하는 while문
        while (packet.win_flag == 0)
        {
            while (1) {
                // 가위 바위 보 진행 -> 가위바위보중 하나 입력받기.
                printf("\n\t@@@@@@@@@@@@@@@\t\t\t가위바위보 게임을 진행합니다\t\t\t@@@@@@@@@@@@@@@\n");
                printf("\n<< 가위, 바위, 보, 승률확인, 종료 >> 중 하나를 입력하세요 : ");
                if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
                    break;

                // '\n' 문자 제거
                len = strlen(buf);
                if (buf[len - 1] == '\n')
                    buf[len - 1] = '\0';
                if (strlen(buf) == 0)
                    break;

                char scissors[5] = "가위";
                char rock[5] = "바위";
                char paper[3] = "보";
                char end_buf[5] = "종료";
                char win_rate_buf[13] = "승률확인";

                if (strcmp(buf, scissors) == 0) {
                    packet.rps = GAWI;                                                                  // 가위라면 rps 필드에 0
                    break;
                }
                else if (strcmp(buf, rock) == 0) {
                    packet.rps = BAWEE;                                                                 // 바위라면 rps 필드에 1
                    break;
                }
                else if (strcmp(buf, paper) == 0) {
                    packet.rps = BO;                                                                    // 보라면 rps 필드에 2
                    break;
                }
                else if (strcmp(buf, win_rate_buf) == 0) {
                    return_game_rate(packet);                                                           // 입력받은 문자열이 '승률확인'이라면 승률 함수 호출
                }
                else if (strcmp(buf, end_buf) == 0) {
                    packet.end_flag = 1;                                                                // 입력받은 문자열이 '종료'라면 end_flag를 1로 세트해서 게임 종료를 요청하고 최종 결과 출력
                    send(sock, (char*)&packet, sizeof(packet), 0);
                    printf("\n||||\t\t사용자의 요청으로 게임을 종료합니다. Good Bye!\t\t||||\n");
                    printf("||||||||최종  승률||||||||\n");
                    return_game_rate(packet);
                    break;
                }
                else {
                    printf("<< 가위, 바위, 보, 승률확인, 종료 >> 중에 하나만 입력하셔야합니다.\n");     // 잘못된 문자열 입력 시 예외처리
                }
            }

            // 가위바위보 게임 단계에서 종료를 입력했을 때, 클라이언트 소켓 종료를 위한 조건문
            if (packet.end_flag == 1) {
                break;
            }

            // 가위바위보 정보 보내기 가위 0, 바위 1, 보 2
            retval = send(sock, (char*)&packet, sizeof(packet), 0);
            if (retval == SOCKET_ERROR) {
                err_display("send()");
                break;
            }
            //printf("[TCP 클라이언트] %d바이트를 보냈습니다.\n", retval);

            // 무승부, 승, 패 여부 받기. packet.win_flag
            retval = recv(sock, (char*)&packet, sizeof(packet), 0);
            if (retval == SOCKET_ERROR) {
                err_display("recv()");
                break;
            }
            else if (retval == 0)
                break;

            //printf("[TCP 클라이언트] %d바이트를 받았습니다.\n", retval);
            // printf("%d\n", packet.win_flag); // 승리 플래그 확인하기

            // win_flag가 0이라면. 즉, 가위바위보가 무승부라면
            if (packet.win_flag == 0) {
                if (packet.rps == GAWI)
                    printf("\n|| 클라 : 가위 || 서버 : 가위 || 무승부 || 가위바위보를 다시 진행합니다.\n");
                else if (packet.rps == BAWEE)
                    printf("\n|| 클라 : 바위 || 서버 : 바위 || 무승부 || 가위바위보를 다시 진행합니다\n");
                else if (packet.rps == BO)
                    printf("\n||  클라 : 보  ||  서버 : 보  || 무승부 || 가위바위보를 다시 진행합니다\n");
            }

            // win_flag가 1이라면. 즉, 클라이언트가 패배했다면(묵찌빠 방어)
            else if (packet.win_flag == 1) {
                if (packet.rps == GAWI)
                    printf("\n|| 클라 : 가위 || 서버 : 바위 || 클라 패배 || <방어>로 시작합니다\n");
                else if (packet.rps == BAWEE)
                    printf("\n|| 클라 : 바위 ||  서버 : 보  || 클라 패배 || <방어>로 시작합니다\n");
                else if (packet.rps == BO)
                    printf("\n||  클라 : 보  || 서버 : 가위 || 클라 패배 || <방어>로 시작합니다\n");
            }

            // win_flag가 2라면. 즉, 클라이언트가 승리했다면(묵찌빠 공격)
            else if (packet.win_flag == 2) {
                if (packet.rps == GAWI)
                    printf("\n|| 클라 : 가위 ||  서버 : 보  || 클라 승리 || <공격>으로 시작합니다\n");
                else if (packet.rps == BAWEE)
                    printf("\n|| 클라 : 바위 || 서버 : 가위 || 클라 승리 || <공격>으로 시작합니다\n");
                else if (packet.rps == BO)
                    printf("\n||  클라 : 보  || 서버 : 바위 || 클라 승리 || <공격>으로 시작합니다\n");
            }
        }

        // packet.data에 저장 된 묵찌빠 시작 메시지 전송받음
        retval = recv(sock, (char*)&packet, sizeof(packet), 0);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        }
        else if (retval == 0)
            break;
        retval = strlen(packet.data);

        // 묵찌빠 시작메시지 출력
        printf("\n\t@@@@@@@@@@@@@@@\t\t\t%s\t\t\t@@@@@@@@@@@@@@@\n", packet.data);

        // 묵찌빠 게임 시작(클라가 가위바위보 승리하거나 패배했을 때)
        if (packet.win_flag == 1 || packet.win_flag == 2) {
            int temp_setnum = packet.setnum_flag;                                                           // 현재 플레이한 게임 횟수를 temp_setnum에 저장

            // 묵찌빠 게임이 종료되면 게임 횟수인 packet.setnum_flag가 1 증가하기 때문에, 위에서 저장한 플레이 횟수와 달라지면 묵찌빠 게임 종료
            while (temp_setnum == packet.setnum_flag) {
                // 묵찌빠 게임 시작
                while (1) {
                    printf("\n<< 묵, 찌, 빠, 승률확인, 종료 >> 중 하나를 입력하세요 :  ");
                    if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
                        break;

                    // '\n' 문자 제거
                    len = strlen(buf);
                    if (buf[len - 1] == '\n')
                        buf[len - 1] = '\0';
                    if (strlen(buf) == 0)
                        break;

                    char scissors[3] = "찌";
                    char rock[3] = "묵";
                    char paper[3] = "빠";
                    char end_buf[5] = "종료";
                    char win_rate_buf[13] = "승률확인";

                    if (strcmp(buf, scissors) == 0) {
                        packet.rps = GAWI;                                                                  // 입력 값이 '찌'라면 rps 필드에 0
                        break;
                    }
                    else if (strcmp(buf, rock) == 0) {
                        packet.rps = BAWEE;                                                                 // 입력 값이 '묵'이라면 rps 필드에 1
                        break;
                    }
                    else if (strcmp(buf, paper) == 0) {
                        packet.rps = BO;                                                                    // 입력 값이 '빠'라면 rps 필드에 2
                        break;
                    }
                    else if (strcmp(buf, win_rate_buf) == 0) {
                        return_game_rate(packet);                                                           // 입력 값이 '승률확인'이라면 현재 승률 출력
                    }
                    else if (strcmp(buf, end_buf) == 0) {
                        packet.end_flag = 1;                                                                // 입력 값이 '종료'라면 게임 종료 메시지와 최종 승률 출력
                        printf("\n|| 사용자의 요청으로 게임을 종료합니다. Good Bye! ||\n");
                        printf("||||||||최종  승률||||||||\n");
                        return_game_rate(packet);
                        break; // 묵찌빠 입력받는 무한루프 탈출
                    }
                    else {
                        printf("<< 묵, 찌, 빠, 승률확인, 종료 >> 중에 하나만 입력하셔야합니다.\n");         // 잘못된 문자열 입력 시 예외처리
                    }
                }

                // 가위바위보 정보 서버로 보내기. 찌 0, 묵 1, 빠 2
                retval = send(sock, (char*)&packet, sizeof(packet), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                    break;
                }
                //printf("[TCP 클라이언트] %d바이트를 보냈습니다.\n", retval);

                if (packet.end_flag == 1) {
                    break; // 묵찌빠 입력 단계에서 종료가 입력되면 while(temp_setnum == packet.setnum_flag) 탈출
                }

                // 패킷에 저장된 승, 패 여부와 문자열 받기. packet.win_rate_flag
                retval = recv(sock, (char*)&packet, sizeof(packet), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("recv()");
                    break;
                }
                else if (retval == 0)
                    break;

                //printf("[TCP 클라이언트] %d바이트를 받았습니다.\n", retval);
                //printf("%d\n", packet.client_win_flag); // 승리 플래그 전달 받았는지 확인하기
                //printf("%d\n", packet.setnum_flag); // 세트 수 전달 받았는지 확인하기
                printf("\n%s", packet.data); // 전달받은 결과 출력
            }
        }
    }

    // closesocket()
    closesocket(sock);

    // 윈속 종료
    WSACleanup();
    return 0;
}

// 요구사항 : 서버 연결 기능, 클라이언트 연결 기능, 클라이언트 관리 기능