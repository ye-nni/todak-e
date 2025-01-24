import cv2
from http.server import BaseHTTPRequestHandler, HTTPServer
from picamera2 import Picamera2
from threading import Thread
from datetime import datetime
import time
import sys
from threading import Timer

class StreamingHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/stream":
            self.send_response(200)
            picamera2.start() # 카메라 시작
            self.send_header('Content-Type', 'multipart/x-mixed-replace; boundary=FRAME')
            self.end_headers()
            try:
                while True:
                    # 카메라에서 프레임 가져오기
                    frame = picamera2.capture_array("main")
                    frame_rgb = cv2.cvtColor(frame, cv2.COLOR_YUV2GRAY_YUYV)
                    _, jpeg_data = cv2.imencode(".jpg", frame_rgb)
    
                    # 클라이언트로 스트리밍
                    self.wfile.write(b"--FRAME\r\n")
                    self.send_header("Content-Type", "image/jpeg")
                    self.send_header("Content-Length", len(jpeg_data))
                    self.end_headers()
                    self.wfile.write(jpeg_data.tobytes())
            except BrokenPipeError:
                picamera2.stop()
                print("HTTP 클라이언트 연결이 끊어졌습니다.")
        elif self.path == "/record":
            # 레코딩 요청 처리
            self.send_response(200)
            self.end_headers()
            Thread(target=self.handle_recording).start()
        elif self.path == "/report":
            # 아기 상태 확인
            self.send_response(200)
            self.send_header('Content-type', 'text/html; charset=utf-8')
            self.end_headers()
            with open('baby_status.txt', 'r', encoding='utf-8') as file:
                content = file.read()
            html = f"<pre>{content}</pre>"
            self.wfile.write(html.encode('utf-8'))
        else:
            self.send_error(404)
            self.end_headers()

    def handle_recording(self):
        check = 0
        try:
            picamera2.start() # 카메라가 시작되지 않은 경우 카메라 시작
        except:
            check = 1
            print("카메라가 이미 실행 중입니다")
            
        print("영상 녹화 시작...")
        current_time = datetime.now()
        file_name = current_time.strftime("%Y-%m-%d_%H-%M.yuyv")
        with open(file_name, "wb") as f:
            start_time = time.time()
            while time.time() - start_time < 10:  # 10초 동안 녹화
                frame = picamera2.capture_array("main")
                f.write(frame)
        print(f"Recording saved as {file_name}")
        if check == 0:
            picamera2.stop()

def handle_sigterm(): # 종료를 위한 핸들러
    global server, picamera2
    print("파이썬 스트리밍 프로세스 종료 시작...")
    try:
        picamera2.stop()
    except:
        print("카메라가 이미 중지된 상태입니다.")
    server.shutdown()
    print("파이썬 스트리밍 프로세스 종료 완료")
    sys.exit()

class ThreadedHTTPServer(HTTPServer):
    """멀티스레드 HTTP 서버"""
    def process_request(self, request, client_address):
        Thread(target=self.__new_request, args=(request, client_address), daemon=True).start()

    def __new_request(self, request, client_address):
        try:
            self.finish_request(request, client_address)
        except Exception as e:
            print(f"Error handling request from {client_address}: {e}")
        finally:
            self.shutdown_request(request)

if __name__ == "__main__":
    global server, picamera2
    picamera2 = Picamera2()
    picamera2.configure(picamera2.create_video_configuration(main={"size": (640, 480), "format": "YUYV"}))

    server = ThreadedHTTPServer(('192.168.1.2', 8000), StreamingHandler)  

    print("스트리밍 서버가 실행 중입니다.")

    try:
        while True:
            server.serve_forever()  # 서버가 계속 실행되도록 유지
    except KeyboardInterrupt:
        print("파이썬 스트리밍 프로세스 종료 대기...")
        t = Timer(5, handle_sigterm)  
        t.start()
