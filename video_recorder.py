import requests

def request_recording():
    """서버에 레코딩 요청"""
    print("레코딩 요청 중...")
    try:
        # 서버의 레코딩 API 호출
        response = requests.get("http://192.168.1.2:8000/record")
        if response.status_code == 200:
            print("레코딩이 성공적으로 요청되었습니다.")
        else:
            print("레코딩 요청 실패:", response.status_code)
    except Exception as e:
        print("레코딩 요청 중 오류 발생:", e)

if __name__ == "__main__":
    request_recording()

