#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>

#define BLUE_LED 4
#define GREEN_LED 17
#define YELLOW_LED 27
#define RED_LED 22
#define BUTTON 20
#define MOTOR 26
#define SERVER_PORT 8081
#define BUFFER_SIZE 256

int state = 1;
int stop_thread = 0;
int server_sock, client_sock;
struct sockaddr_in server_addr, client_addr;
socklen_t client_addr_len;
char buffer[BUFFER_SIZE];
int bytes_received;

void write_to_file() {   // 웹에 올린 파일을 작성
    FILE *file = fopen("baby_status.txt", "w");
    if (file == NULL) {
        perror("텍스트 파일 열기 실패");
        exit(EXIT_FAILURE);
    }

    // 측정된 시간을 기록
    time_t now = time(NULL);
    now -= 1; // 통신 시간을 감안해 1초를 뺌
    struct tm *t = localtime(&now);
    char time_buffer[100];
    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", t);

    // 받은 아기 상태를 파일에 씀
    fprintf(file, "측정 시간: %s(업데이트 하려면 새로고침 하세요)\n", time_buffer);
    fprintf(file, "아기 상태: 상태 번호 %s\n", buffer);
    fprintf(file, "\n");
    fclose(file);
}

void *inputThread(void *arg) {
    while (!stop_thread) {
        while ((bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';  // 문자열 끝에 NULL 추가
            printf("받은 데이터: %s\n", buffer);
            state = buffer[0] - '0';    // 상태 번호 추출 후 integer로 변환
            write_to_file(); // 아기 상태를 텍스트 파일에 쓰는 함수
        }

        if (bytes_received < 0) {
            perror("데이터 수신 실패");
        }
    }
    return NULL;
}

void initialize() { // GPIO 초기 세팅
    gpioSetMode(BLUE_LED, PI_OUTPUT);
    gpioSetMode(GREEN_LED, PI_OUTPUT);
    gpioSetMode(YELLOW_LED, PI_OUTPUT);
    gpioSetMode(RED_LED, PI_OUTPUT);
    gpioSetMode(MOTOR, PI_OUTPUT);
}

void turnOffOutputs() { // 모든 Output 끄기
    gpioWrite(BLUE_LED, 0);
    gpioWrite(GREEN_LED, 0);
    gpioWrite(YELLOW_LED, 0);
    gpioWrite(RED_LED, 0);
    gpioWrite(MOTOR, 0);
}

void motorControl(int s) { // 모터 및 영상 녹화 컨트롤
    int buttonState;

    if (s == 2) {
        while (1) {
            buttonState = gpioRead(BUTTON);
            if (state == 3) break; // 상태 3이 들어올 경우 나가기
            else if (buttonState == 0) { // 버튼이 눌렸을 경우 모터를 끄고 LED를 바꿈
                gpioWrite(MOTOR, 0);
                gpioWrite(YELLOW_LED, 0);
                state = 1;
                break;
            } 
            else { // 버튼이 눌리지 않았으면 진동
                gpioWrite(MOTOR, 1);
                usleep(200000);
                gpioWrite(MOTOR, 0);
                usleep(200000);
            }
        }
    } 
    else if (s == 3) {
        FILE *python_process = popen("python3 video_recorder.py", "w"); // 아기 상태 3일 경우 녹화 시작
        if (python_process == NULL) {
            perror("녹화 실패");
        }
        pclose(python_process);
        while (1) {
            buttonState = gpioRead(BUTTON);
            if (buttonState == 0) { // 버튼이 눌렸을 경우 모터를 끄고 LED를 바꿈
                gpioWrite(MOTOR, 0);
                gpioWrite(RED_LED, 0);
                state = 1;
                break;
            } 
            else { // 버튼이 눌리지 않았으면 진동
                gpioWrite(MOTOR, 1);
                usleep(500000);
            }
        }
    }
    
}

void turnOnOuputs(int s) { // 상태에 따른 LED 컨트롤
    switch (s) {
        case 0:
            gpioWrite(MOTOR, 0);
            gpioWrite(BLUE_LED, 1);
            gpioWrite(YELLOW_LED, 0);
            gpioWrite(GREEN_LED, 0);
            gpioWrite(RED_LED, 0);
            break;
        case 1:
            gpioWrite(MOTOR, 0);
            gpioWrite(GREEN_LED, 1);
            gpioWrite(BLUE_LED, 0);
            gpioWrite(YELLOW_LED, 0);
            gpioWrite(RED_LED, 0);
            break;
        case 2:
            gpioWrite(BLUE_LED, 0);
            gpioWrite(GREEN_LED, 0);
            gpioWrite(RED_LED, 0);
            gpioWrite(YELLOW_LED, 1);
            motorControl(2);
            break;
        case 3:
            gpioWrite(BLUE_LED, 0);
            gpioWrite(GREEN_LED, 0);
            gpioWrite(YELLOW_LED, 0);
            gpioWrite(RED_LED, 1);
            motorControl(3);
            break;
        default:
            break;
    }
}

int setupSocket() { 
    // 소켓 생성
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("소켓 생성 실패");
        return -1;
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 소켓 바인딩
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("바인딩 실패");
        close(server_sock);
        return -1;
    }

    // 연결 대기
    if (listen(server_sock, 5) < 0) {
        perror("연결 대기 실패");
        close(server_sock);
        return -1;
    }

    printf("서버가 준비되었습니다. 클라이언트의 연결을 기다리는 중...\n");

    // 클라이언트 연결 수락
    client_addr_len = sizeof(client_addr);
    if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
        perror("연결 수락 실패");
        close(server_sock);
        return -1;
    }

    printf("클라이언트가 연결되었습니다.\n");
}

void sigint_handler(int signum) { // 종료를 위한 핸들러
    printf("\n서버 프로그램 종료 시작...\n");
    stop_thread = 1; // 스레드를 종료하기 위함
}

int main() {
    if (setupSocket() < 0) {
        perror("소켓 설정 실패");
        return 1;
    }

    pthread_t thread;
    FILE *python_process = popen("python3 streaming_server.py", "w"); // 카메라 스트리밍 시작
    if (python_process == NULL) perror("파이썬 스트리밍 프로그램 시작 실패");

    if (gpioInitialise() < 0) {
        fprintf(stderr, "Pigpio fail");
        return 1;
    }

    pthread_create(&thread, NULL, inputThread, NULL);  // 데이터를 받아오는 쓰레드
    initialize();
    gpioSetMode(BUTTON, PI_INPUT);  // 버튼 입력 설정

    signal(SIGINT, sigint_handler);
    while (1) { // 상태에 따라 led, motor 제어
        turnOnOuputs(state);
        if (stop_thread == 1) {
            goto cleanup;
        }
    }

cleanup:
    printf("소켓 클라이언트 종료 대기...\n");
    pthread_join(thread, NULL);
    printf("소켓 클라이언트 종료 완료\n");
    turnOffOutputs();
    gpioTerminate();
    pclose(python_process);
    printf("서버 프로그램 종료 완료\n");

    return 0;
}
