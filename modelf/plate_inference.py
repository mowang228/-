import sys
import json
import cv2
import numpy as np
import onnxruntime as ort

sys.stdout.reconfigure(encoding='utf-8')

def imread_unicode(path):
    """支持中文路径的 cv2.imread 替代函数"""
    return cv2.imdecode(np.fromfile(path, dtype=np.uint8), cv2.IMREAD_COLOR)

plateName = "#京沪津渝冀晋蒙辽吉黑苏浙皖闽赣鲁豫鄂湘粤桂琼川贵云藏陕甘青宁新学警港澳挂使领民航危0123456789ABCDEFGHJKLMNPQRSTUVWXYZ险品"
plate_color_list = ['黑色', '蓝色', '绿色', '白色', '黄色']
mean_value, std_value = 0.588, 0.193

color_classes = ["黄色", "橙色", "绿色", "灰色", "红色", "蓝色", "白色", "金色", "棕色", "黑色"]
vehicle_classes = ["轿车", "SUV", "面包车", "两厢车", "MPV", "皮卡", "客车", "货车", "旅行车"]
mean = [0.485, 0.456, 0.406]
std = [0.229, 0.224, 0.225]

class PlateDetector:
    def __init__(self, model_path):
        self.session = ort.InferenceSession(model_path, providers=['CPUExecutionProvider'])
        self.input_name = self.session.get_inputs()[0].name
        self.output_names = [o.name for o in self.session.get_outputs()]
        self.input_shape = self.session.get_inputs()[0].shape
        self.input_h = self.input_shape[2]
        self.input_w = self.input_shape[3]
        self.conf_threshold = 0.5
        self.iou_threshold = 0.45
    
    def preprocess(self, image):
        h, w = image.shape[:2]
        img = cv2.resize(image, (self.input_w, self.input_h))
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        img = img.astype(np.float32) / 255.0
        img = np.transpose(img, (2, 0, 1))
        img = np.expand_dims(img, axis=0)
        return img, h, w
    
    def detect(self, image_path):
        image = imread_unicode(image_path)
        if image is None:
            return []
        
        img, h, w = self.preprocess(image)
        outputs = self.session.run(self.output_names, {self.input_name: img})
        
        plates = []
        if len(outputs) > 0:
            output = outputs[0]
            if output.ndim == 3:
                boxes = output[0]
                valid_boxes = []
                
                for box in boxes:
                    if len(box) >= 5:
                        x_center, y_center, width, height, conf = box[:5]
                        
                        if conf > self.conf_threshold:
                            x1 = int((x_center - width / 2) / self.input_w * w)
                            y1 = int((y_center - height / 2) / self.input_h * h)
                            x2 = int((x_center + width / 2) / self.input_w * w)
                            y2 = int((y_center + height / 2) / self.input_h * h)
                            x1 = max(0, x1)
                            y1 = max(0, y1)
                            x2 = min(w, x2)
                            y2 = min(h, y2)
                            if x2 > x1 and y2 > y1:
                                valid_boxes.append({
                                    'x1': x1, 'y1': y1, 'x2': x2, 'y2': y2,
                                    'confidence': float(conf)
                                })
                
                plates = self.nms(valid_boxes)
        
        return plates
    
    def nms(self, boxes):
        if len(boxes) == 0:
            return []
        
        boxes.sort(key=lambda x: x['confidence'], reverse=True)
        selected = []
        
        for box in boxes:
            keep = True
            for selected_box in selected:
                iou = self.calculate_iou(box, selected_box)
                if iou > self.iou_threshold:
                    keep = False
                    break
            if keep:
                selected.append(box)
        
        return selected
    
    def calculate_iou(self, box1, box2):
        x1 = max(box1['x1'], box2['x1'])
        y1 = max(box1['y1'], box2['y1'])
        x2 = min(box1['x2'], box2['x2'])
        y2 = min(box1['y2'], box2['y2'])
        
        w = max(0, x2 - x1)
        h = max(0, y2 - y1)
        area_inter = w * h
        
        area1 = (box1['x2'] - box1['x1']) * (box1['y2'] - box1['y1'])
        area2 = (box2['x2'] - box2['x1']) * (box2['y2'] - box2['y1'])
        
        return area_inter / (area1 + area2 - area_inter) if (area1 + area2 - area_inter) > 0 else 0

class PlateRecognizer:
    def __init__(self, model_path):
        self.session = ort.InferenceSession(model_path, providers=['CPUExecutionProvider'])
        self.input_name = self.session.get_inputs()[0].name
        self.input_shape = self.session.get_inputs()[0].shape
    
    def preprocess(self, image):
        img = cv2.resize(image, (168, 48))
        img = img.astype(np.float32)
        img = (img / 255.0 - mean_value) / std_value
        img = img.transpose(2, 0, 1)
        img = img.reshape(1, *img.shape)
        return img
    
    def decode_plate(self, preds):
        pre = 0
        new_preds = []
        for i in range(len(preds)):
            if preds[i] != 0 and preds[i] != pre:
                new_preds.append(preds[i])
            pre = preds[i]
        plate = ""
        for i in new_preds:
            if int(i) < len(plateName):
                plate += plateName[int(i)]
        return plate
    

    def recognize(self, image_path, plate_box=None):
        image = imread_unicode(image_path)
        if image is None:
            return {"plate_text": "", "plate_color": "未知"}
        
        if plate_box is not None:
            x1, y1, x2, y2 = plate_box['x1'], plate_box['y1'], plate_box['x2'], plate_box['y2']
            plate_region = image[y1:y2, x1:x2]
            if plate_region.size == 0:
                return {"plate_text": "", "plate_color": "未知"}
        else:
            plate_region = image
        
        img = self.preprocess(plate_region)
        outputs = self.session.run(None, {self.input_name: img})
        
        if len(outputs) == 0:
            return {"plate_text": "", "plate_color": "未知"}
        
        y_onnx_plate = outputs[0]
        y_onnx_color = outputs[1] if len(outputs) > 1 else None
        
        index = np.argmax(y_onnx_plate, axis=-1)
        plate_no = self.decode_plate(index[0])
        
        plate_color = "未知"
        if y_onnx_color is not None:
            index_color = np.argmax(y_onnx_color)
            if index_color < len(plate_color_list):
                plate_color = plate_color_list[index_color]
        
        return {"plate_text": plate_no, "plate_color": plate_color}


def _classify_color_hsv(vehicle_region):
    """HSV 颜色分类：对模型输出做后处理校验"""
    if vehicle_region.size == 0:
        return None, 0.0
    
    hsv = cv2.cvtColor(vehicle_region, cv2.COLOR_BGR2HSV)
    h, s, v = cv2.split(hsv)
    
    mean_s = np.mean(s)
    mean_v = np.mean(v)
    
    if mean_s < 30:
        if mean_v < 60:
            return "黑色", 0.85
        elif mean_v > 200:
            return "白色", 0.85
        else:
            return "灰色", 0.80
    
    mean_h = np.mean(h)
    if mean_h < 8 or mean_h > 175:
        return "红色", 0.75
    elif mean_h < 25:
        return "橙色", 0.75
    elif mean_h < 40:
        return "黄色", 0.75
    elif mean_h < 80:
        return "绿色", 0.75
    elif mean_h < 130:
        return "蓝色", 0.75
    elif mean_h < 155:
        return "蓝色", 0.65
    else:
        return "红色", 0.65


def _classify_type_by_aspect(vehicle_region):
    """通过宽高比验证车型"""
    if vehicle_region.size == 0:
        return None, 0.0
    rh, rw = vehicle_region.shape[:2]
    aspect = rw / max(rh, 1)
    
    if aspect > 1.8:
        return "轿车", 0.50
    elif aspect > 1.4:
        return "SUV", 0.45
    elif aspect > 1.1:
        return "面包车", 0.45
    else:
        return "客车", 0.45


class VehicleAttributeDetector:
    def __init__(self, model_path):
        self.session = ort.InferenceSession(model_path, providers=['CPUExecutionProvider'])
        self.input_name = self.session.get_inputs()[0].name
        self.input_shape = self.session.get_inputs()[0].shape
        self.inpHeight = self.input_shape[2]
        self.inpWidth = self.input_shape[3]
    
    def preprocess(self, image):
        img = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        img = cv2.resize(img, (self.inpWidth, self.inpHeight), interpolation=cv2.INTER_LINEAR)
        return img
    
    def normalize_(self, img):
        row = img.shape[0]
        col = img.shape[1]
        input_image = np.zeros(row * col * 3, dtype=np.float32)
        for c in range(3):
            for i in range(row):
                for j in range(col):
                    pix = img[i, j, c]
                    input_image[c * row * col + i * col + j] = (pix / 255.0 - mean[c]) / std[c]
        return input_image
    
    def _extract_vehicle_region(self, image, plate_box):
        """从车牌框智能扩展提取车辆区域"""
        h, w = image.shape[:2]
        px1, py1, px2, py2 = plate_box['x1'], plate_box['y1'], plate_box['x2'], plate_box['y2']
        plate_w = px2 - px1
        plate_h = py2 - py1
        
        # 动态扩展系数：车牌占比越小（远处车辆），扩展比例越大
        plate_area_ratio = (plate_w * plate_h) / (w * h)
        expand_h = 3.8 if plate_area_ratio < 0.008 else 3.0
        expand_w = max(1.6, min(3.5, w / plate_w * 0.5))
        
        new_w = int(plate_w * expand_w)
        new_h = int(plate_h * expand_h)
        
        # 车牌上方占 65%，下方占 35%（车主要在车牌上方）
        vx1 = max(0, int(px1 - (new_w - plate_w) / 2))
        vy1 = max(0, int(py1 - new_h * 0.65))
        vx2 = min(w, int(px2 + (new_w - plate_w) / 2))
        vy2 = min(h, int(vy1 + new_h))
        
        if vx2 > vx1 and vy2 > vy1:
            return image[vy1:vy2, vx1:vx2]
        return image
    
    def detect(self, image_path, plate_box=None):
        image = imread_unicode(image_path)
        if image is None:
            return {"color": "unknown", "vehicle_type": "unknown", "color_confidence": 0.0, "vehicle_type_confidence": 0.0}
        
        h, w = image.shape[:2]
        
        # 提取车辆区域（无 plate_box 时用全图）
        if plate_box is not None:
            vehicle_region = self._extract_vehicle_region(image, plate_box)
        else:
            vehicle_region = image
        
        # 模型推理
        dstimg = self.preprocess(vehicle_region)
        input_image = self.normalize_(dstimg)
        
        input_shape = [1, 3, self.inpHeight, self.inpWidth]
        input_tensor = input_image.reshape(input_shape)
        
        outputs = self.session.run(None, {self.input_name: input_tensor})
        
        if len(outputs) == 0:
            return {"color": "unknown", "vehicle_type": "unknown", "color_confidence": 0.0, "vehicle_type_confidence": 0.0}
        
        pdata = outputs[0][0]
        
        color_idx = np.argmax(pdata[:10])
        type_idx = np.argmax(pdata[10:])
        
        model_color = color_classes[color_idx]
        model_type = vehicle_classes[type_idx]
        color_conf = float(pdata[color_idx])
        type_conf = float(pdata[type_idx + 10])
        
        # ---- 后处理 1：HSV 颜色校验 ----
        hsv_color, hsv_conf = _classify_color_hsv(vehicle_region)
        final_color = model_color
        final_color_conf = color_conf
        if hsv_color is not None:
            if color_conf < 0.5:
                # 模型置信度低 → 用 HSV
                final_color = hsv_color
                final_color_conf = hsv_conf
            elif hsv_color != model_color and hsv_conf > 0.82 and color_conf < 0.7:
                # HSV 很确定但模型给出不同结果 → 用 HSV
                final_color = hsv_color
                final_color_conf = hsv_conf
        
        # ---- 后处理 2：车型宽高比验证 ----
        asp_type, asp_conf = _classify_type_by_aspect(vehicle_region)
        final_type = model_type
        final_type_conf = type_conf
        if asp_type is not None and type_conf < 0.5:
            final_type = asp_type
            final_type_conf = asp_conf
        
        return {
            "color": final_color,
            "vehicle_type": final_type,
            "color_confidence": final_color_conf,
            "vehicle_type_confidence": final_type_conf
        }


def main():
    if len(sys.argv) < 4:
        print(json.dumps({'error': 'Usage: python plate_inference.py <mode> <model_path> <image_path> [plate_box]'}))
        return
    
    mode = sys.argv[1]
    model_path = sys.argv[2]
    image_path = sys.argv[3]
    
    try:
        if mode == 'detect':
            detector = PlateDetector(model_path)
            plates = detector.detect(image_path)
            result = {'success': True, 'plates': plates}
        elif mode == 'recognize':
            recognizer = PlateRecognizer(model_path)
            
            plate_box = None
            if len(sys.argv) >= 8:
                plate_box = {
                    'x1': int(sys.argv[4]),
                    'y1': int(sys.argv[5]),
                    'x2': int(sys.argv[6]),
                    'y2': int(sys.argv[7])
                }
            
            rec_result = recognizer.recognize(image_path, plate_box)
            result = {'success': True, 'plate_text': rec_result['plate_text'], 'plate_color': rec_result['plate_color']}
        elif mode == 'vehicle_attr':
            detector = VehicleAttributeDetector(model_path)
            
            plate_box = None
            if len(sys.argv) >= 8:
                plate_box = {
                    'x1': int(sys.argv[4]),
                    'y1': int(sys.argv[5]),
                    'x2': int(sys.argv[6]),
                    'y2': int(sys.argv[7])
                }
            
            attr_result = detector.detect(image_path, plate_box)
            result = {'success': True, **attr_result}
        else:
            result = {'success': False, 'error': 'Unknown mode: ' + mode}
        
        print(json.dumps(result, ensure_ascii=False))
    except Exception as e:
        import traceback
        print(json.dumps({'success': False, 'error': str(e), 'traceback': traceback.format_exc()}))

if __name__ == '__main__':
    main()
