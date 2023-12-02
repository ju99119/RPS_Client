#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� VC++ ������ �� ��� ����
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

// ���� �Լ� ���� ��� �� ����
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

// ���� �Լ� ���� ���
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

// ����� ���� ������ ���� �Լ�
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

// ������ Ŭ���̾�Ʈ ���� �ְ� �޴� ��Ŷ ����ü ����
#pragma pack(1)
typedef struct {
    int rps = 0; // Ŭ���̾�Ʈ�� ������ ������������ ���� �÷����̴�.        0 : ����(��), 1 : ����(��), 2 : ��(��)
    int status = 0; // ó�� ���� ���� �� �� �����͸� �޴� �ʵ�.              0 : ���� ���� ��� ����, 1 : ���� ����!
    int client_win_flag = 0; // Ŭ���̾�Ʈ�� �¸��� ������ 1�� ����.         Ŭ���̾�Ʈ�� �� �� �̰������ ���� Flag
    int setnum_flag = 0; // ����� ���� ���� ������ 1�� ����.                ����� ������ �� �� �ߴ����� ���� Flag
    int end_flag = 0; // ���� �÷����̴�.                                    0 : default, 1 : 1�� ������ Ŭ���̾�Ʈ ���� �ٷ� ����
    int win_flag = 0; // ���������� ��� �÷���.                             0 : ���º�, 1 : Ŭ���̾�Ʈ �й�(����� ���), 2 : Ŭ���̾�Ʈ �¸�(����� ����)
    char data[64]; // ������ ���� �ʵ�
} RPS;
#pragma pack()

// ���� ����� ������ִ� �Լ�
void return_game_rate(RPS pack) {
    if (pack.setnum_flag == 0) {
        printf("������ �� �ǵ� �������� �ʾƼ� ���� ����� ��µ��� �ʽ��ϴ�.\n"); // �� �ǵ� �÷������� ���� ��쿡�� ������� ���ڿ� ���
    }
    // �·� ����Ͽ� �� ���� ���� �¸� ��, �й� ��, �·��� ���
    else {
        printf("||||||||||||||||||||||||||\n");
        printf("||\t�÷��� : %d��\t||\n", pack.setnum_flag);
        printf("||\t��  �� : %d��\t||\n", pack.client_win_flag);
        int lose = (pack.setnum_flag - pack.client_win_flag);
        printf("||\t��  �� : %d��\t||\n", lose);
        float rate = ((float)pack.client_win_flag / pack.setnum_flag);
        printf("||\t��  �� : %.2f%%\t||\n", rate * 100);
        printf("||||||||||||||||||||||||||\n");
    }
}

int main(int argc, char* argv[])
{
    int retval;

    // ���� �ʱ�ȭ
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

    // ������ ��ſ� ����� ����
    char buf[BUFSIZE + 1];
    int len;

    RPS packet; // ������ Ŭ���̾�Ʈ ���� �ְ� �޴� ��Ŷ ����ü ����

    int start_err_flag = 0; // ó���� '����' �Է� �ÿ� �ٸ� ���� �Է� �Ǿ��� ���� ����ó���� ���� ����
    // ������ ������ ���

    while (1) {

        while (1) {
            if (start_err_flag == 0) {
                retval = recv(sock, (char*)&packet, sizeof(packet), 0);                                      // "KJH ����� ����" ���ڿ��� �޾ƿ��� ����
                if (retval == SOCKET_ERROR) {
                    err_display("recv()");
                    break;
                }
                else if (retval == 0)
                    break;
                printf("\n\t%s\n", packet.data);                                                             // "KJH ����� ����" ���
            }
            printf("������ �����Ͻ÷��� \"����\"�̶�� �Է����ּ���('�·�Ȯ��', '����'�� ����) : ");         // �������� ���� ���� ��û�ϱ�
            if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
                break;

            // '\n' ���� ����
            len = strlen(buf);
            if (buf[len - 1] == '\n')
                buf[len - 1] = '\0';
            if (strlen(buf) == 0)
                break;

            char start_buf[5] = "����";
            char end_buf[5] = "����";
            char win_rate_buf[9] = "�·�Ȯ��";

            // �Է¹��� ���ڿ��� '����'�̶�� status �ʵ带 1�� ��Ʈ�ؼ� ���� ������ ��û
            if (strcmp(buf, start_buf) == 0) {
                printf("\n\t@@@@@@@@@@@@@@@\t\t\t\t������ �����մϴ�.\t\t\t@@@@@@@@@@@@@@@");
                packet.status = 1;
                packet.win_flag = 0;
                retval = send(sock, (char*)&packet, sizeof(packet), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                }
                start_err_flag = 0;                                                                          // ������ �ݺ��ϱ� ������, ���� ������ ���� error_flag �ʱ�ȭ
                break;
            }
            // �Է¹��� ���ڿ��� '����'��� end_flag�� 1�� ��Ʈ�ؼ� ���� ���Ḧ ��û�ϰ� ���� ��� ���
            else if (strcmp(buf, end_buf) == 0) {
                packet.end_flag = 1;
                send(sock, (char*)&packet, sizeof(packet), 0);
                printf("\n||||\t\t������� ��û���� ������ �����մϴ�. Good Bye!\t\t||||\n");
                printf("||||||||����  �·�||||||||\n");
                return_game_rate(packet);                                                                    // ���� ���� �ÿ� ���� ��� ���
                break;
            }
            // �Է¹��� ���ڿ��� '�·�Ȯ��'�̶�� �·� �Լ� ȣ��
            else if (strcmp(buf, win_rate_buf) == 0) {
                return_game_rate(packet);
                start_err_flag = 1;                                                                          // ���� ������ ���� �ܰ��̱� ������, �·�Ȯ�ε� ������ error�� �Ǵ��Ͽ� error_flag 1�� ��Ʈ
            }
            else {
                printf("\"����\"�̶�� �Է��ϼž� ������ ���۵˴ϴ�. ������ �����ϰ� �����ôٸ� \"����\"�� �Է����ּ���.\n");
                start_err_flag = 1;                                                                          // error_flag 1�� ��Ʈ�Ͽ� ������ �ùٸ� ���ڿ� �Է��ϵ��� ��
            }
        }

        // ���� ���� ��û �ܰ迡�� ���Ḧ �Է����� ��, Ŭ���̾�Ʈ ���� ���Ḧ ���� ���ǹ�
        if (packet.end_flag == 1)
            break;

        // ���������� ���� �����ϴ� while��
        while (packet.win_flag == 0)
        {
            while (1) {
                // ���� ���� �� ���� -> ������������ �ϳ� �Է¹ޱ�.
                printf("\n\t@@@@@@@@@@@@@@@\t\t\t���������� ������ �����մϴ�\t\t\t@@@@@@@@@@@@@@@\n");
                printf("\n<< ����, ����, ��, �·�Ȯ��, ���� >> �� �ϳ��� �Է��ϼ��� : ");
                if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
                    break;

                // '\n' ���� ����
                len = strlen(buf);
                if (buf[len - 1] == '\n')
                    buf[len - 1] = '\0';
                if (strlen(buf) == 0)
                    break;

                char scissors[5] = "����";
                char rock[5] = "����";
                char paper[3] = "��";
                char end_buf[5] = "����";
                char win_rate_buf[13] = "�·�Ȯ��";

                if (strcmp(buf, scissors) == 0) {
                    packet.rps = GAWI;                                                                  // ������� rps �ʵ忡 0
                    break;
                }
                else if (strcmp(buf, rock) == 0) {
                    packet.rps = BAWEE;                                                                 // ������� rps �ʵ忡 1
                    break;
                }
                else if (strcmp(buf, paper) == 0) {
                    packet.rps = BO;                                                                    // ����� rps �ʵ忡 2
                    break;
                }
                else if (strcmp(buf, win_rate_buf) == 0) {
                    return_game_rate(packet);                                                           // �Է¹��� ���ڿ��� '�·�Ȯ��'�̶�� �·� �Լ� ȣ��
                }
                else if (strcmp(buf, end_buf) == 0) {
                    packet.end_flag = 1;                                                                // �Է¹��� ���ڿ��� '����'��� end_flag�� 1�� ��Ʈ�ؼ� ���� ���Ḧ ��û�ϰ� ���� ��� ���
                    send(sock, (char*)&packet, sizeof(packet), 0);
                    printf("\n||||\t\t������� ��û���� ������ �����մϴ�. Good Bye!\t\t||||\n");
                    printf("||||||||����  �·�||||||||\n");
                    return_game_rate(packet);
                    break;
                }
                else {
                    printf("<< ����, ����, ��, �·�Ȯ��, ���� >> �߿� �ϳ��� �Է��ϼž��մϴ�.\n");     // �߸��� ���ڿ� �Է� �� ����ó��
                }
            }

            // ���������� ���� �ܰ迡�� ���Ḧ �Է����� ��, Ŭ���̾�Ʈ ���� ���Ḧ ���� ���ǹ�
            if (packet.end_flag == 1) {
                break;
            }

            // ���������� ���� ������ ���� 0, ���� 1, �� 2
            retval = send(sock, (char*)&packet, sizeof(packet), 0);
            if (retval == SOCKET_ERROR) {
                err_display("send()");
                break;
            }
            //printf("[TCP Ŭ���̾�Ʈ] %d����Ʈ�� ���½��ϴ�.\n", retval);

            // ���º�, ��, �� ���� �ޱ�. packet.win_flag
            retval = recv(sock, (char*)&packet, sizeof(packet), 0);
            if (retval == SOCKET_ERROR) {
                err_display("recv()");
                break;
            }
            else if (retval == 0)
                break;

            //printf("[TCP Ŭ���̾�Ʈ] %d����Ʈ�� �޾ҽ��ϴ�.\n", retval);
            // printf("%d\n", packet.win_flag); // �¸� �÷��� Ȯ���ϱ�

            // win_flag�� 0�̶��. ��, ������������ ���ºζ��
            if (packet.win_flag == 0) {
                if (packet.rps == GAWI)
                    printf("\n|| Ŭ�� : ���� || ���� : ���� || ���º� || ������������ �ٽ� �����մϴ�.\n");
                else if (packet.rps == BAWEE)
                    printf("\n|| Ŭ�� : ���� || ���� : ���� || ���º� || ������������ �ٽ� �����մϴ�\n");
                else if (packet.rps == BO)
                    printf("\n||  Ŭ�� : ��  ||  ���� : ��  || ���º� || ������������ �ٽ� �����մϴ�\n");
            }

            // win_flag�� 1�̶��. ��, Ŭ���̾�Ʈ�� �й��ߴٸ�(����� ���)
            else if (packet.win_flag == 1) {
                if (packet.rps == GAWI)
                    printf("\n|| Ŭ�� : ���� || ���� : ���� || Ŭ�� �й� || <���>�� �����մϴ�\n");
                else if (packet.rps == BAWEE)
                    printf("\n|| Ŭ�� : ���� ||  ���� : ��  || Ŭ�� �й� || <���>�� �����մϴ�\n");
                else if (packet.rps == BO)
                    printf("\n||  Ŭ�� : ��  || ���� : ���� || Ŭ�� �й� || <���>�� �����մϴ�\n");
            }

            // win_flag�� 2���. ��, Ŭ���̾�Ʈ�� �¸��ߴٸ�(����� ����)
            else if (packet.win_flag == 2) {
                if (packet.rps == GAWI)
                    printf("\n|| Ŭ�� : ���� ||  ���� : ��  || Ŭ�� �¸� || <����>���� �����մϴ�\n");
                else if (packet.rps == BAWEE)
                    printf("\n|| Ŭ�� : ���� || ���� : ���� || Ŭ�� �¸� || <����>���� �����մϴ�\n");
                else if (packet.rps == BO)
                    printf("\n||  Ŭ�� : ��  || ���� : ���� || Ŭ�� �¸� || <����>���� �����մϴ�\n");
            }
        }

        // packet.data�� ���� �� ����� ���� �޽��� ���۹���
        retval = recv(sock, (char*)&packet, sizeof(packet), 0);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        }
        else if (retval == 0)
            break;
        retval = strlen(packet.data);

        // ����� ���۸޽��� ���
        printf("\n\t@@@@@@@@@@@@@@@\t\t\t%s\t\t\t@@@@@@@@@@@@@@@\n", packet.data);

        // ����� ���� ����(Ŭ�� ���������� �¸��ϰų� �й����� ��)
        if (packet.win_flag == 1 || packet.win_flag == 2) {
            int temp_setnum = packet.setnum_flag;                                                           // ���� �÷����� ���� Ƚ���� temp_setnum�� ����

            // ����� ������ ����Ǹ� ���� Ƚ���� packet.setnum_flag�� 1 �����ϱ� ������, ������ ������ �÷��� Ƚ���� �޶����� ����� ���� ����
            while (temp_setnum == packet.setnum_flag) {
                // ����� ���� ����
                while (1) {
                    printf("\n<< ��, ��, ��, �·�Ȯ��, ���� >> �� �ϳ��� �Է��ϼ��� :  ");
                    if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
                        break;

                    // '\n' ���� ����
                    len = strlen(buf);
                    if (buf[len - 1] == '\n')
                        buf[len - 1] = '\0';
                    if (strlen(buf) == 0)
                        break;

                    char scissors[3] = "��";
                    char rock[3] = "��";
                    char paper[3] = "��";
                    char end_buf[5] = "����";
                    char win_rate_buf[13] = "�·�Ȯ��";

                    if (strcmp(buf, scissors) == 0) {
                        packet.rps = GAWI;                                                                  // �Է� ���� '��'��� rps �ʵ忡 0
                        break;
                    }
                    else if (strcmp(buf, rock) == 0) {
                        packet.rps = BAWEE;                                                                 // �Է� ���� '��'�̶�� rps �ʵ忡 1
                        break;
                    }
                    else if (strcmp(buf, paper) == 0) {
                        packet.rps = BO;                                                                    // �Է� ���� '��'��� rps �ʵ忡 2
                        break;
                    }
                    else if (strcmp(buf, win_rate_buf) == 0) {
                        return_game_rate(packet);                                                           // �Է� ���� '�·�Ȯ��'�̶�� ���� �·� ���
                    }
                    else if (strcmp(buf, end_buf) == 0) {
                        packet.end_flag = 1;                                                                // �Է� ���� '����'��� ���� ���� �޽����� ���� �·� ���
                        printf("\n|| ������� ��û���� ������ �����մϴ�. Good Bye! ||\n");
                        printf("||||||||����  �·�||||||||\n");
                        return_game_rate(packet);
                        break; // ����� �Է¹޴� ���ѷ��� Ż��
                    }
                    else {
                        printf("<< ��, ��, ��, �·�Ȯ��, ���� >> �߿� �ϳ��� �Է��ϼž��մϴ�.\n");         // �߸��� ���ڿ� �Է� �� ����ó��
                    }
                }

                // ���������� ���� ������ ������. �� 0, �� 1, �� 2
                retval = send(sock, (char*)&packet, sizeof(packet), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                    break;
                }
                //printf("[TCP Ŭ���̾�Ʈ] %d����Ʈ�� ���½��ϴ�.\n", retval);

                if (packet.end_flag == 1) {
                    break; // ����� �Է� �ܰ迡�� ���ᰡ �ԷµǸ� while(temp_setnum == packet.setnum_flag) Ż��
                }

                // ��Ŷ�� ����� ��, �� ���ο� ���ڿ� �ޱ�. packet.win_rate_flag
                retval = recv(sock, (char*)&packet, sizeof(packet), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("recv()");
                    break;
                }
                else if (retval == 0)
                    break;

                //printf("[TCP Ŭ���̾�Ʈ] %d����Ʈ�� �޾ҽ��ϴ�.\n", retval);
                //printf("%d\n", packet.client_win_flag); // �¸� �÷��� ���� �޾Ҵ��� Ȯ���ϱ�
                //printf("%d\n", packet.setnum_flag); // ��Ʈ �� ���� �޾Ҵ��� Ȯ���ϱ�
                printf("\n%s", packet.data); // ���޹��� ��� ���
            }
        }
    }

    // closesocket()
    closesocket(sock);

    // ���� ����
    WSACleanup();
    return 0;
}

// �䱸���� : ���� ���� ���, Ŭ���̾�Ʈ ���� ���, Ŭ���̾�Ʈ ���� ���