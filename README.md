## 부모님께 알림 파트

- 센서 및 액츄에이터
  
1. 카메라
2. LED 4개(파랑, 초록, 노랑, 빨강)
3. 진동 모터
4. 버튼


- 사용 GPIO핀

|라즈베리파이 핀|연결 센서/액츄에이터|
|:--:|:--:|
|CSI|카메라|
|GPIO4|파랑 LED|
|GPIO17|초록 LED|
|GPIO27|노랑 LED|
|GPIO22|빨강 LED|
|GPIO26|진동 모터|
|GPIO20|버튼|

- 코드 설명

|파일명|역할|
|:--:|:--:|
|support.c|메인 컨트롤, 소켓 서버. 상태에 따른 LED 및 모터 컨트롤|
|streaming_server.py|카메라 스트리밍, 영상 녹화,  상태 파일 HTTP 호스팅|
|video_recorder.py|영상 녹화 요청|


- 사용 라이브러리
  
|support.c|
|:--:|
|pigpio.h|
|stdio.h|
|stdlib.h|
|string.h|
|unistd.h|
|pthread.h|
|signal.h|
|arpa/inet.h|

|streaming_server.py|
|:--:|
|cv2 (Python OpenCV 라이브러리)|
|http.server (Python HTTP 서버 라이브러리)|
|Picamera2 (Raspberry Pi 카메라 제어 라이브러리)|
|threading (Python 쓰레드 라이브러리)|
|datetime (Python 날짜 및 시간 라이브러리)|
|time (Python 시간 관련 라이브러리)|
|sys (Python 시스템 관련 라이브러리)|

|video_recorder.py|
|:--:|
|requests (Python HTTP 요청 라이브러리)|
