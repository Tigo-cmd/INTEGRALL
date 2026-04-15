import cv2
import requests
import numpy as np

# 🏷️ 1. Configurations
# Replace this with the IP printed in your Arduino Serial Monitor!
ESP32_CAM_IP = "192.168.1.168" 
STREAM_URL = f"http://{ESP32_CAM_IP}:81/stream"

def run_analysis():
    print(f"🚀 Connecting to Integrall Stream at {STREAM_URL}...")
    
    # Setup for Motion Detection
    # Initialize the first frame
    first_frame = None

    try:
        # Use streaming mode to fetch raw MJPEG bytes
        stream = requests.get(STREAM_URL, stream=True, timeout=10)
        if stream.status_code != 200:
            print(f"❌ Connection Failed. HTTP Status: {stream.status_code}")
            return

        bytes_data = bytes()
        print("✅ Stream Online! Detection Running... (Press 'q' to quit)")

        for chunk in stream.iter_content(chunk_size=1024):
            bytes_data += chunk
            
            # Find the JPEG markers in the byte stream
            a = bytes_data.find(b'\xff\xd8')
            b = bytes_data.find(b'\xff\xd9')
            
            if a != -1 and b != -1:
                # 🖼️ Extract and decode the JPEG frame
                jpg_frame = bytes_data[a:b+2]
                bytes_data = bytes_data[b+2:]
                
                # Convert bytes to a numpy array for OpenCV
                frame = cv2.imdecode(np.frombuffer(jpg_frame, dtype=np.uint8), cv2.IMREAD_COLOR)
                
                if frame is not None:
                    # 🚀 MOTION DETECTION LOGIC (Pure OpenCV)
                    
                    # 1. Convert to gray and blur slightly to remove sensor noise
                    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                    gray = cv2.GaussianBlur(gray, (21, 21), 0)

                    # 2. Store the first frame for comparison
                    if first_frame is None:
                        first_frame = gray
                        continue

                    # 3. Calculate difference between current and first frame
                    frame_delta = cv2.absdiff(first_frame, gray)
                    thresh = cv2.threshold(frame_delta, 25, 255, cv2.THRESH_BINARY)[1]

                    # 4. Dilate the threshold to fill in holes
                    thresh = cv2.dilate(thresh, None, iterations=2)
                    
                    # 5. Find Contours (the moving objects)
                    cnts, _ = cv2.findContours(thresh.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

                    # 6. Loop over contours and draw Bounding Boxes
                    for c in cnts:
                        # If the contour is too small, ignore it
                        if cv2.contourArea(c) < 5000:
                            continue

                        # Compute the bounding box and draw it on the frame
                        (x, y, w, h) = cv2.boundingRect(c)
                        cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)
                        cv2.putText(frame, "MOTION DETECTED", (10, 20),
                                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)

                    # 📺 Display the Result
                    cv2.imshow("Integrall - Bounding Box Stream", frame)
                
                # Close script with 'q'
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break

    except Exception as e:
        print(f"❌ Error: {e}")
    finally:
        cv2.destroyAllWindows()
        print("🛑 Stream Closed.")

if __name__ == "__main__":
    run_analysis()
