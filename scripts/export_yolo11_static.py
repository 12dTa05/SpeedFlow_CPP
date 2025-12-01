#!/usr/bin/env python3
"""
Export YOLO11 model with STATIC batch size for DeepStream
This fixes the TensorRT dynamic shape error
"""

from ultralytics import YOLO

# Load model
model = YOLO('/home/mta/Documents/IoT_Graduate/yolo11n.pt')

# Export with STATIC shape (dynamic=False is key!)
model.export(
    format='onnx',
    dynamic=False,      # CRITICAL: Static batch size
    simplify=True,
    opset=12,
    imgsz=640
)

print("\n✅ Model exported successfully!")
print("Output: yolo11n.onnx (static batch)")
print("\nMove this file to SpeedFlow_CPP/configs/ and update config_infer_primary_yolo11.txt")
