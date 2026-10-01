#!/usr/bin/env python3
"""
EtherCAT Triple Servo Controller & Live Monitor UI
==================================================
Graphical User Interface (PyQt) untuk pemantauan dan kontrol interaktif
3 unit motor servo Lichuan LC10E melalui EtherCAT bus secara real-time.

Fitur Utama:
  - Dukungan penuh 3 Servo Motor (Slave 1, Slave 2, Slave 3).
  - Mode Hold-to-Jog: Tekan terus tombol [MAJU]/[MUNDUR] untuk berputar pelan,
    lepas tombol langsung berhenti otomatis dengan ramp pengereman halus.
  - Slider Kecepatan 0 - 1000 RPM dengan proteksi & peringatan visual jika RPM tinggi
    (sangat aman untuk motor di atas meja yang belum dibuatkan mounting).
  - Preset kecepatan cepat: 15 RPM (Sangat Pelan), 30 RPM (Pelan/Aman), 60 RPM (Sedang).
  - Tombol Kontrol Serentak (Maju/Mundur Semua Motor Bersama).
  - Gauge Dial Sudut Poros (0 - 360°) untuk masing-masing 3 motor.
  - Osiloskop waveform 3-trace real-time (Cyan, Emerald, Amber) tanpa dependensi luar.
  - Tombol Reset Fault CiA402 langsung dari antarmuka.
  - Mode Simulasi (Demo) interaktif penuh untuk uji coba tanpa perangkat keras.
"""

import sys
import os
import re
import math
import time
from collections import deque

# ── Universal Qt Import Wrapper (PyQt5 / PyQt6 / PySide6 / PySide2) ──
QT_LIB = None
try:
    from PyQt5 import QtCore, QtGui, QtWidgets
    from PyQt5.QtCore import Qt, QTimer, pyqtSignal, QProcess, QPointF
    from PyQt5.QtGui import QPainter, QColor, QPen, QBrush, QFont, QLinearGradient, QPolygonF
    from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout,
                                 QHBoxLayout, QLabel, QPushButton, QFrame, QLineEdit,
                                 QGridLayout, QTextEdit, QCheckBox, QMessageBox,
                                 QSlider, QSpinBox, QGroupBox)
    QT_LIB = "PyQt5"
except ImportError:
    try:
        from PyQt6 import QtCore, QtGui, QtWidgets
        from PyQt6.QtCore import Qt, QTimer, pyqtSignal, QProcess, QPointF
        from PyQt6.QtGui import QPainter, QColor, QPen, QBrush, QFont, QLinearGradient, QPolygonF
        from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout,
                                     QHBoxLayout, QLabel, QPushButton, QFrame, QLineEdit,
                                     QGridLayout, QTextEdit, QCheckBox, QMessageBox,
                                     QSlider, QSpinBox, QGroupBox)
        QT_LIB = "PyQt6"
    except ImportError:
        try:
            from PySide6 import QtCore, QtGui, QtWidgets
            from PySide6.QtCore import Qt, QTimer, Signal as pyqtSignal, QProcess, QPointF
            from PySide6.QtGui import QPainter, QColor, QPen, QBrush, QFont, QLinearGradient, QPolygonF
            from PySide6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout,
                                         QHBoxLayout, QLabel, QPushButton, QFrame, QLineEdit,
                                         QGridLayout, QTextEdit, QCheckBox, QMessageBox,
                                         QSlider, QSpinBox, QGroupBox)
            QT_LIB = "PySide6"
        except ImportError:
            try:
                from PySide2 import QtCore, QtGui, QtWidgets
                from PySide2.QtCore import Qt, QTimer, Signal as pyqtSignal, QProcess, QPointF
                from PySide2.QtGui import QPainter, QColor, QPen, QBrush, QFont, QLinearGradient, QPolygonF
                from PySide2.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout,
                                             QHBoxLayout, QLabel, QPushButton, QFrame, QLineEdit,
                                             QGridLayout, QTextEdit, QCheckBox, QMessageBox,
                                             QSlider, QSpinBox, QGroupBox)
                QT_LIB = "PySide2"
            except ImportError:
                QT_LIB = None

if QT_LIB is None:
    print("\n" + "="*60)
    print(" [PERINGATAN] Library GUI Qt belum terpasang di sistem Python.")
    print("="*60)
    print(" Silakan pasang PyQt5 dengan menjalankan perintah di terminal:")
    print("     sudo apt update && sudo apt install -y python3-pyqt5")
    print("="*60 + "\n")
    sys.exit(1)

PULSES_PER_REV = 131072.0  # Lichuan LC10E 17-bit single-turn resolution


# ─────────────────────────────────────────────────────────────────────────────
# 1. Custom Visual Dial Widget: Sudut Poros Motor (0 - 360 Derajat)
# ─────────────────────────────────────────────────────────────────────────────
class EncoderDialWidget(QWidget):
    """Dial visual interaktif penunjuk sudut mekanik poros motor 0-360 derajat."""
    def __init__(self, color_theme="#00e5ff", parent=None):
        super().__init__(parent)
        self.setMinimumSize(120, 120)
        self.angle_deg = 0.0
        self.theme_color = QColor(color_theme)

    def set_angle(self, deg):
        self.angle_deg = deg % 360.0
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing if hasattr(QPainter, 'RenderHint') else QPainter.Antialiasing)

        w = self.width()
        h = self.height()
        side = min(w, h)
        painter.translate(w / 2.0, h / 2.0)
        painter.scale(side / 140.0, side / 140.0)

        # Outer bezel
        painter.setPen(QPen(QColor("#272c3d"), 3))
        painter.setBrush(QBrush(QColor("#131622")))
        painter.drawEllipse(-62, -62, 124, 124)

        # Inner track
        painter.setPen(QPen(QColor("#1a1e2d"), 4))
        painter.drawEllipse(-52, -52, 104, 104)

        # Degree ticks
        painter.setPen(QPen(QColor("#4e5879"), 1))
        for i in range(12):
            angle = i * 30
            rad = math.radians(angle)
            x1 = 49 * math.sin(rad)
            y1 = -49 * math.cos(rad)
            x2 = 57 * math.sin(rad)
            y2 = -57 * math.cos(rad)
            painter.drawLine(int(x1), int(y1), int(x2), int(y2))

        # Arc track for current angle
        arc_pen = QPen(self.theme_color, 4)
        arc_pen.setCapStyle(Qt.PenCapStyle.RoundCap if hasattr(Qt, 'PenCapStyle') else Qt.RoundCap)
        painter.setPen(arc_pen)
        span_angle = int(-self.angle_deg * 16)
        painter.drawArc(-52, -52, 104, 104, 90 * 16, span_angle)

        # Needle pointer
        painter.save()
        painter.rotate(self.angle_deg)
        needle = QPolygonF([QPointF(0, -48), QPointF(5, -8), QPointF(0, 0), QPointF(-5, -8)])
        painter.setPen(Qt.PenStyle.NoPen if hasattr(Qt, 'PenStyle') else Qt.NoPen)
        painter.setBrush(QBrush(self.theme_color))
        painter.drawPolygon(needle)
        painter.restore()

        # Center cap
        painter.setPen(QPen(QColor("#ffffff"), 1))
        painter.setBrush(QBrush(QColor("#1c2030")))
        painter.drawEllipse(-10, -10, 20, 20)

        # Center angle text
        painter.setPen(QColor("#ffffff"))
        font = QFont("Monospace", 9, QFont.Weight.Bold if hasattr(QFont, 'Weight') else QFont.Bold)
        painter.setFont(font)
        text = f"{self.angle_deg:.1f}°"
        painter.drawText(-30, 22, 60, 20, Qt.AlignmentFlag.AlignCenter if hasattr(Qt, 'AlignmentFlag') else Qt.AlignCenter, text)


# ─────────────────────────────────────────────────────────────────────────────
# 2. Custom Real-Time Waveform Graph (3-Trace Triple Servo Oscilloscope)
# ─────────────────────────────────────────────────────────────────────────────
class LiveWaveformWidget(QWidget):
    """Grafik osiloskop triple-trace untuk pergerakan real-time ketiga motor."""
    def __init__(self, max_points=240, parent=None):
        super().__init__(parent)
        self.setMinimumHeight(140)
        self.max_points = max_points
        self.data1 = deque(maxlen=max_points)
        self.data2 = deque(maxlen=max_points)
        self.data3 = deque(maxlen=max_points)
        for _ in range(max_points):
            self.data1.append(0.0)
            self.data2.append(0.0)
            self.data3.append(0.0)

    def add_points(self, p1, p2, p3):
        self.data1.append(float(p1))
        self.data2.append(float(p2))
        self.data3.append(float(p3))
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing if hasattr(QPainter, 'RenderHint') else QPainter.Antialiasing)

        w = self.width()
        h = self.height()

        # Background grid
        painter.fillRect(0, 0, w, h, QColor("#0d1017"))
        painter.setPen(QPen(QColor("#1a202c"), 1))
        for y in range(20, h, 28):
            painter.drawLine(0, y, w, y)
        for x in range(30, w, 40):
            painter.drawLine(x, 0, x, h)

        all_vals = list(self.data1) + list(self.data2) + list(self.data3)
        min_v = min(all_vals)
        max_v = max(all_vals)
        if abs(max_v - min_v) < 100:
            min_v -= 50
            max_v += 50
        margin = (max_v - min_v) * 0.1
        min_v -= margin
        max_v += margin

        span = max_v - min_v if max_v != min_v else 1.0

        def to_y(val):
            ratio = (val - min_v) / span
            return h - 10 - ratio * (h - 20)

        step_x = w / float(self.max_points - 1)

        # Zero reference line
        if min_v <= 0 <= max_v:
            y_zero = to_y(0)
            painter.setPen(QPen(QColor("#313b53"), 1, Qt.PenStyle.DashLine if hasattr(Qt, 'PenStyle') else Qt.DashLine))
            painter.drawLine(0, int(y_zero), w, int(y_zero))

        # Trace 1: Motor 1 (Cyan)
        pen1 = QPen(QColor("#00e5ff"), 2)
        painter.setPen(pen1)
        for i in range(len(self.data1) - 1):
            painter.drawLine(QPointF(i * step_x, to_y(self.data1[i])),
                             QPointF((i + 1) * step_x, to_y(self.data1[i + 1])))

        # Trace 2: Motor 2 (Neon Emerald)
        pen2 = QPen(QColor("#00e676"), 2)
        painter.setPen(pen2)
        for i in range(len(self.data2) - 1):
            painter.drawLine(QPointF(i * step_x, to_y(self.data2[i])),
                             QPointF((i + 1) * step_x, to_y(self.data2[i + 1])))

        # Trace 3: Motor 3 (Amber Gold)
        pen3 = QPen(QColor("#ff9100"), 2)
        painter.setPen(pen3)
        for i in range(len(self.data3) - 1):
            painter.drawLine(QPointF(i * step_x, to_y(self.data3[i])),
                             QPointF((i + 1) * step_x, to_y(self.data3[i + 1])))

        # Legend & Scale
        font = QFont("Monospace", 8)
        painter.setFont(font)
        painter.setPen(QColor("#6b7a99"))
        painter.drawText(8, 15, f"MAX: {max_v:+,.0f}")
        painter.drawText(8, h - 5, f"MIN: {min_v:+,.0f}")

        # Legend badges
        painter.setPen(QColor("#00e5ff"))
        painter.drawText(w - 290, 15, "■ S1 (Cyan)")
        painter.setPen(QColor("#00e676"))
        painter.drawText(w - 200, 15, "■ S2 (Emerald)")
        painter.setPen(QColor("#ff9100"))
        painter.drawText(w - 105, 15, "■ S3 (Amber)")


# ─────────────────────────────────────────────────────────────────────────────
# 3. Servo Card Widget (Data, Status & Hold-to-Jog 1 Motor)
# ─────────────────────────────────────────────────────────────────────────────
class ServoCard(QFrame):
    """Panel tampilan lengkap data dan kontrol Hold-to-Jog satu motor servo."""
    def __init__(self, slave_id, title, color_hex, parent_win, parent=None):
        super().__init__(parent)
        self.slave_id = slave_id
        self.title = title
        self.color_hex = color_hex
        self.parent_win = parent_win
        self.offset = 0
        self.last_pos = 0
        self.last_time = time.time()
        self.calc_rpm = 0.0

        self.setStyleSheet(f"""
            QFrame {{
                background-color: #141724;
                border: 1px solid #23283c;
                border-radius: 8px;
            }}
        """)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 10, 12, 10)
        layout.setSpacing(6)

        # Header Title
        title_box = QHBoxLayout()
        lbl_title = QLabel(title)
        lbl_title.setStyleSheet(f"color: {color_hex}; font-size: 13px; font-weight: bold; border: none;")
        self.lbl_id = QLabel(f"SLAVE #{slave_id}")
        self.lbl_id.setStyleSheet(f"color: #7b85aa; font-size: 10px; font-weight: bold; border: 1px solid #2b324a; border-radius: 3px; padding: 2px 5px;")
        title_box.addWidget(lbl_title)
        title_box.addStretch()
        title_box.addWidget(self.lbl_id)
        layout.addLayout(title_box)

        # Middle Box: Digital Readouts + Visual Dial
        mid_layout = QHBoxLayout()

        grid = QGridLayout()
        grid.setVerticalSpacing(2)
        grid.setHorizontalSpacing(8)

        grid.addWidget(self._make_label("POSISI AKTUAL:", "#8d98c2"), 0, 0, 1, 2)
        self.val_pos = QLabel("+0")
        self.val_pos.setStyleSheet(f"color: {color_hex}; font-size: 18px; font-weight: bold; font-family: Monospace; border: none;")
        grid.addWidget(self.val_pos, 1, 0, 1, 2)

        grid.addWidget(self._make_label("PUTARAN:", "#8d98c2"), 2, 0)
        self.val_turns = QLabel("+0.00 rot")
        self.val_turns.setStyleSheet("color: #ffffff; font-size: 12px; font-weight: bold; font-family: Monospace; border: none;")
        grid.addWidget(self.val_turns, 2, 1)

        grid.addWidget(self._make_label("SUDUT POROS:", "#8d98c2"), 3, 0)
        self.val_deg = QLabel("0.0°")
        self.val_deg.setStyleSheet(f"color: {color_hex}; font-size: 12px; font-weight: bold; font-family: Monospace; border: none;")
        grid.addWidget(self.val_deg, 3, 1)

        grid.addWidget(self._make_label("SPEED (RPM):", "#8d98c2"), 4, 0)
        self.val_rpm = QLabel("0 RPM")
        self.val_rpm.setStyleSheet("color: #ffffff; font-size: 12px; font-weight: bold; font-family: Monospace; border: none;")
        grid.addWidget(self.val_rpm, 4, 1)

        grid.addWidget(self._make_label("TORSI:", "#8d98c2"), 5, 0)
        self.val_torque = QLabel("0.0 %")
        self.val_torque.setStyleSheet("color: #ffab00; font-size: 12px; font-weight: bold; font-family: Monospace; border: none;")
        grid.addWidget(self.val_torque, 5, 1)

        mid_layout.addLayout(grid, stretch=3)

        # Dial Widget
        self.dial = EncoderDialWidget(color_theme=color_hex)
        mid_layout.addWidget(self.dial, stretch=2)
        layout.addLayout(mid_layout)

        # Status Badges
        badge_box = QHBoxLayout()
        badge_box.setSpacing(3)
        self.badge_ready = self._make_badge("RDY")
        self.badge_swon = self._make_badge("SWON")
        self.badge_enabled = self._make_badge("EN")
        self.badge_target = self._make_badge("TGT")
        self.badge_fault = self._make_badge("FLT", is_fault=True)

        badge_box.addWidget(self.badge_ready)
        badge_box.addWidget(self.badge_swon)
        badge_box.addWidget(self.badge_enabled)
        badge_box.addWidget(self.badge_target)
        badge_box.addWidget(self.badge_fault)
        layout.addLayout(badge_box)

        # ── Hold-to-Jog Control Panel Khusus Motor Ini ──
        ctrl_box = QVBoxLayout()
        ctrl_box.setSpacing(4)

        # Indikator Status Gerak
        self.lbl_motion_status = QLabel("⏸ DIAM (STANDBY)")
        self.lbl_motion_status.setAlignment(Qt.AlignmentFlag.AlignCenter if hasattr(Qt, 'AlignmentFlag') else Qt.AlignCenter)
        self.lbl_motion_status.setStyleSheet("background-color: #1a1e2d; color: #7f8cae; font-size: 10px; font-weight: bold; padding: 3px; border-radius: 4px; border: none;")
        ctrl_box.addWidget(self.lbl_motion_status)

        # Tombol Hold-to-Jog Maju & Mundur
        jog_btn_box = QHBoxLayout()
        jog_btn_box.setSpacing(6)

        self.btn_ccw = QPushButton("◀ MUNDUR")
        self.btn_ccw.setToolTip(f"Tekan terus untuk memutar mundur Motor {slave_id} secara pelan. Lepas untuk berhenti.")
        self.btn_ccw.setStyleSheet(f"""
            QPushButton {{
                background-color: #1e2336;
                color: #cdd6f4;
                font-weight: bold;
                font-size: 11px;
                padding: 8px 6px;
                border-radius: 5px;
                border: 1px solid #363e5e;
            }}
            QPushButton:hover {{
                background-color: #29304a;
                border-color: {color_hex};
                color: #ffffff;
            }}
            QPushButton:pressed {{
                background-color: #c62828;
                color: #ffffff;
                border-color: #ff5252;
            }}
        """)
        self.btn_ccw.pressed.connect(self._on_ccw_pressed)
        self.btn_ccw.released.connect(self._on_jog_released)
        jog_btn_box.addWidget(self.btn_ccw)

        self.btn_cw = QPushButton("MAJU ▶")
        self.btn_cw.setToolTip(f"Tekan terus untuk memutar maju Motor {slave_id} secara pelan. Lepas untuk berhenti.")
        self.btn_cw.setStyleSheet(f"""
            QPushButton {{
                background-color: #1e2336;
                color: #cdd6f4;
                font-weight: bold;
                font-size: 11px;
                padding: 8px 6px;
                border-radius: 5px;
                border: 1px solid #363e5e;
            }}
            QPushButton:hover {{
                background-color: #29304a;
                border-color: {color_hex};
                color: #ffffff;
            }}
            QPushButton:pressed {{
                background-color: #2e7d32;
                color: #ffffff;
                border-color: #4caf50;
            }}
        """)
        self.btn_cw.pressed.connect(self._on_cw_pressed)
        self.btn_cw.released.connect(self._on_jog_released)
        jog_btn_box.addWidget(self.btn_cw)

        ctrl_box.addLayout(jog_btn_box)

        # Tombol Step 1 Rotasi (131.072 pulsa)
        step_btn_box = QHBoxLayout()
        step_btn_box.setSpacing(6)

        self.btn_step_ccw = QPushButton("↶ 1 Putaran (-)")
        self.btn_step_ccw.setToolTip("Putar mundur tepat 1 rotasi penuh (131.072 pulsa)")
        self.btn_step_ccw.setStyleSheet("""
            QPushButton {
                background-color: #1a233a;
                color: #90caf9;
                font-size: 9px;
                font-weight: bold;
                padding: 4px;
                border-radius: 4px;
                border: 1px solid #283e6b;
            }
            QPushButton:hover {
                background-color: #213c6b;
                color: #ffffff;
            }
        """)
        self.btn_step_ccw.clicked.connect(lambda: self.parent_win.step_servo(self.slave_id, -131072))
        step_btn_box.addWidget(self.btn_step_ccw)

        self.btn_step_cw = QPushButton("1 Putaran (+) ↷")
        self.btn_step_cw.setToolTip("Putar maju tepat 1 rotasi penuh (131.072 pulsa)")
        self.btn_step_cw.setStyleSheet("""
            QPushButton {
                background-color: #1a233a;
                color: #90caf9;
                font-size: 9px;
                font-weight: bold;
                padding: 4px;
                border-radius: 4px;
                border: 1px solid #283e6b;
            }
            QPushButton:hover {
                background-color: #213c6b;
                color: #ffffff;
            }
        """)
        self.btn_step_cw.clicked.connect(lambda: self.parent_win.step_servo(self.slave_id, 131072))
        step_btn_box.addWidget(self.btn_step_cw)

        ctrl_box.addLayout(step_btn_box)

        # Tombol Mini Tare
        self.btn_tare = QPushButton("🎯 Set Titik Nol")
        self.btn_tare.setStyleSheet("""
            QPushButton {
                background-color: #181d2c;
                color: #8da2d8;
                font-size: 9px;
                font-weight: bold;
                padding: 3px 6px;
                border-radius: 3px;
                border: 1px solid #283049;
            }
            QPushButton:hover {
                background-color: #252d45;
                color: #ffffff;
            }
        """)
        self.btn_tare.clicked.connect(lambda: self.zero_tare(self.last_pos + self.offset))
        ctrl_box.addWidget(self.btn_tare)

        layout.addLayout(ctrl_box)

    def _make_label(self, text, color):
        lbl = QLabel(text)
        lbl.setStyleSheet(f"color: {color}; font-size: 9px; font-weight: bold; border: none;")
        return lbl

    def _make_badge(self, text, is_fault=False):
        lbl = QLabel(text)
        lbl.setAlignment(Qt.AlignmentFlag.AlignCenter if hasattr(Qt, 'AlignmentFlag') else Qt.AlignCenter)
        lbl.setStyleSheet("""
            background-color: #1c2030;
            color: #555e7e;
            font-size: 9px;
            font-weight: bold;
            font-family: Monospace;
            padding: 2px 4px;
            border-radius: 3px;
            border: none;
        """)
        lbl.is_fault = is_fault
        return lbl

    def set_badge_state(self, badge, active):
        if badge.is_fault:
            if active:
                badge.setStyleSheet("background-color: #d50000; color: #ffffff; font-size: 9px; font-weight: bold; font-family: Monospace; padding: 2px 4px; border-radius: 3px; border: none;")
            else:
                badge.setStyleSheet("background-color: #1c2030; color: #555e7e; font-size: 9px; font-weight: bold; font-family: Monospace; padding: 2px 4px; border-radius: 3px; border: none;")
        else:
            if active:
                badge.setStyleSheet(f"background-color: #0d47a1; color: {self.color_hex}; font-size: 9px; font-weight: bold; font-family: Monospace; padding: 2px 4px; border-radius: 3px; border: none;")
            else:
                badge.setStyleSheet("background-color: #1c2030; color: #555e7e; font-size: 9px; font-weight: bold; font-family: Monospace; padding: 2px 4px; border-radius: 3px; border: none;")

    def update_telemetry(self, raw_pos, sw_val, torq_val=0):
        pos = raw_pos - self.offset

        now = time.time()
        dt = now - self.last_time
        if dt >= 0.1:
            d_pos = pos - self.last_pos
            self.calc_rpm = (d_pos / PULSES_PER_REV) * (60.0 / dt)
            self.last_pos = pos
            self.last_time = now

        turns = pos / PULSES_PER_REV
        deg = (pos % int(PULSES_PER_REV)) / PULSES_PER_REV * 360.0
        if deg < 0:
            deg += 360.0

        self.val_pos.setText(f"{pos:+,.0f}")
        self.val_turns.setText(f"{turns:+.2f} rot")
        self.val_deg.setText(f"{deg:.1f}°")
        self.val_rpm.setText(f"{abs(self.calc_rpm):.0f} RPM")
        self.val_torque.setText(f"{torq_val * 0.1:.1f} %")

        self.dial.set_angle(deg)

        self.set_badge_state(self.badge_ready, bool(sw_val & 0x0001))
        self.set_badge_state(self.badge_swon, bool(sw_val & 0x0002))
        self.set_badge_state(self.badge_enabled, bool(sw_val & 0x0004))
        self.set_badge_state(self.badge_target, bool(sw_val & 0x0400))
        self.set_badge_state(self.badge_fault, bool(sw_val & 0x0008))

    def set_motion_status(self, text, active=False, is_cw=True):
        if not active:
            self.lbl_motion_status.setText("⏸ DIAM (STANDBY)")
            self.lbl_motion_status.setStyleSheet("background-color: #1a1e2d; color: #7f8cae; font-size: 10px; font-weight: bold; padding: 3px; border-radius: 4px; border: none;")
        else:
            col = "#1b5e20" if is_cw else "#880e4f"
            txt_col = "#69f0ae" if is_cw else "#ff80ab"
            self.lbl_motion_status.setText(text)
            self.lbl_motion_status.setStyleSheet(f"background-color: {col}; color: {txt_col}; font-size: 10px; font-weight: bold; padding: 3px; border-radius: 4px; border: none;")

    def zero_tare(self, raw_pos):
        self.offset = raw_pos
        self.parent_win.console.append(f"[TARE] Titik nol Motor {self.slave_id} disimpan: offset={raw_pos}")

    def _on_cw_pressed(self):
        if self.parent_win.current_rpm == 0:
            self.parent_win.console.append(f"[INFO] 🛑 Motor {self.slave_id}: RPM = 0. Geser slider kecepatan (misal 30-1000 RPM) untuk memutar motor.")
            return
        self.parent_win.start_jog(self.slave_id, "+")
        self.set_motion_status(f"▶ MAJU (CW) @ {self.parent_win.current_rpm} RPM", active=True, is_cw=True)

    def _on_ccw_pressed(self):
        if self.parent_win.current_rpm == 0:
            self.parent_win.console.append(f"[INFO] 🛑 Motor {self.slave_id}: RPM = 0. Geser slider kecepatan (misal 30-1000 RPM) untuk memutar motor.")
            return
        self.parent_win.start_jog(self.slave_id, "-")
        self.set_motion_status(f"◀ MUNDUR (CCW) @ {self.parent_win.current_rpm} RPM", active=True, is_cw=False)

    def _on_jog_released(self):
        self.parent_win.stop_jog(self.slave_id)
        self.set_motion_status("", active=False)


# ─────────────────────────────────────────────────────────────────────────────
# 4. Main Window Antarmuka Pengujian (PyQt)
# ─────────────────────────────────────────────────────────────────────────────
class TripleServoWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("EtherCAT Triple Servo Controller & Live Monitor - LICHUAN LC10E")
        self.resize(1120, 780)
        self.setStyleSheet("background-color: #0a0d14;")

        self.process = None
        self.is_demo = False
        self.current_rpm = 25  # Default aman: 25 RPM (pelan & halus tanpa mounting)

        # Simulation variables
        self.sim_time = 0.0
        self.sim_active_jogs = {}  # slave_id -> dir (+1 or -1)
        self.sim_positions = {1: 0.0, 2: 0.0, 3: 0.0}

        self.last_raw1 = 0
        self.last_raw2 = 0
        self.last_raw3 = 0

        self._dance_phase = 0
        self._dance_total = 8

        self._init_ui()

        # Timer untuk simulasi / demo mode (50 Hz = 20 ms)
        self.sim_timer = QTimer(self)
        self.sim_timer.timeout.connect(self._update_sim_step)

    def _init_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        main_layout = QVBoxLayout(central)
        main_layout.setContentsMargins(14, 10, 14, 10)
        main_layout.setSpacing(10)

        # ── 1. Top Header Bar ──
        header_frame = QFrame()
        header_frame.setStyleSheet("background-color: #121522; border: 1px solid #1f2538; border-radius: 8px;")
        header_box = QHBoxLayout(header_frame)
        header_box.setContentsMargins(14, 8, 14, 8)

        title_col = QVBoxLayout()
        title_col.setSpacing(2)
        title = QLabel("ETHERCAT TRIPLE SERVO CONTROLLER & ENCODER MONITOR")
        title.setStyleSheet("color: #ffffff; font-size: 15px; font-weight: bold; border: none;")
        subtitle = QLabel("Kontrol Hold-to-Jog 3 Axis & Pemantauan Live Internal Encoder (17-bit / 131.072 Pulsa) Lichuan LC10E")
        subtitle.setStyleSheet("color: #7b88aa; font-size: 11px; border: none;")
        title_col.addWidget(title)
        title_col.addWidget(subtitle)
        header_box.addLayout(title_col)

        header_box.addStretch()

        # Safety & Status Badges
        self.lbl_safety_mode = QLabel("🛡️ MODE AMAN MEJA: PELAN (TANPA MOUNTING)")
        self.lbl_safety_mode.setStyleSheet("background-color: #1b3a24; color: #69f0ae; font-size: 10px; font-weight: bold; padding: 5px 10px; border-radius: 4px; border: 1px solid #2e7d32;")
        header_box.addWidget(self.lbl_safety_mode)

        self.lbl_bus_status = QLabel("STATUS: STANDBY")
        self.lbl_bus_status.setStyleSheet("background-color: #191f32; color: #8da2d8; font-size: 10px; font-weight: bold; padding: 5px 10px; border-radius: 4px; border: 1px solid #2c3654;")
        self.lbl_wkc = QLabel("WKC: - / -")
        self.lbl_wkc.setStyleSheet("background-color: #191f32; color: #8da2d8; font-size: 10px; font-weight: bold; padding: 5px 10px; border-radius: 4px; border: 1px solid #2c3654;")
        header_box.addWidget(self.lbl_bus_status)
        header_box.addWidget(self.lbl_wkc)

        main_layout.addWidget(header_frame)

        # ── 2. Triple Servo Cards (Side-by-Side: S1 Cyan, S2 Emerald, S3 Amber) ──
        cards_layout = QHBoxLayout()
        cards_layout.setSpacing(10)
        self.card1 = ServoCard(1, "MOTOR 1 (AXIS A)", "#00e5ff", self)
        self.card2 = ServoCard(2, "MOTOR 2 (AXIS B)", "#00e676", self)
        self.card3 = ServoCard(3, "MOTOR 3 (AXIS C)", "#ff9100", self)
        cards_layout.addWidget(self.card1)
        cards_layout.addWidget(self.card2)
        cards_layout.addWidget(self.card3)
        main_layout.addLayout(cards_layout)

        # ── 3. Real-Time Oscilloscope Waveform (3 Trace) ──
        wave_frame = QFrame()
        wave_frame.setStyleSheet("background-color: #121522; border: 1px solid #1f2538; border-radius: 8px;")
        wave_box = QVBoxLayout(wave_frame)
        wave_box.setContentsMargins(10, 8, 10, 8)
        lbl_wave = QLabel("TRIPLE-AXIS POSITION WAVEFORM (REAL-TIME OSCILLOSCOPE)")
        lbl_wave.setStyleSheet("color: #7b88aa; font-size: 10px; font-weight: bold; border: none;")
        wave_box.addWidget(lbl_wave)

        self.waveform = LiveWaveformWidget()
        wave_box.addWidget(self.waveform)
        main_layout.addWidget(wave_frame)

        # ── 4. Master Control & Speed Bar ──
        master_frame = QFrame()
        master_frame.setStyleSheet("background-color: #131726; border: 1px solid #242c44; border-radius: 8px;")
        master_box = QVBoxLayout(master_frame)
        master_box.setContentsMargins(12, 10, 12, 10)
        master_box.setSpacing(8)

        # Baris 1: Slider Kecepatan RPM (0 - 3000 RPM)
        speed_row = QHBoxLayout()
        speed_row.setSpacing(8)

        lbl_rpm_title = QLabel("⚡ KECEPATAN (RPM):")
        lbl_rpm_title.setStyleSheet("color: #ffffff; font-weight: bold; font-size: 11px;")
        speed_row.addWidget(lbl_rpm_title)

        self.slider_rpm = QSlider(Qt.Orientation.Horizontal if hasattr(Qt, 'Orientation') else Qt.Horizontal)
        self.slider_rpm.setRange(0, 3000)
        self.slider_rpm.setSingleStep(5)
        self.slider_rpm.setPageStep(50)
        self.slider_rpm.setValue(self.current_rpm)
        self.slider_rpm.setStyleSheet("""
            QSlider::groove:horizontal {
                height: 6px;
                background: #252b42;
                border-radius: 3px;
            }
            QSlider::sub-page:horizontal {
                background: #00e5ff;
                border-radius: 3px;
            }
            QSlider::handle:horizontal {
                background: #ffffff;
                border: 2px solid #00e5ff;
                width: 16px;
                margin-top: -5px;
                margin-bottom: -5px;
                border-radius: 8px;
            }
        """)
        self.slider_rpm.valueChanged.connect(self._on_slider_changed)
        speed_row.addWidget(self.slider_rpm, 1)

        self.spin_rpm = QSpinBox()
        self.spin_rpm.setRange(0, 3000)
        self.spin_rpm.setSingleStep(5)
        self.spin_rpm.setValue(self.current_rpm)
        self.spin_rpm.setSuffix(" RPM")
        self.spin_rpm.setFixedWidth(110)
        self.spin_rpm.setStyleSheet("background-color: #1b2033; color: #00e5ff; font-weight: bold; font-size: 12px; padding: 4px; border-radius: 4px; border: 1px solid #364166;")
        self.spin_rpm.valueChanged.connect(self._on_spin_changed)
        speed_row.addWidget(self.spin_rpm)

        # Quick Presets Buttons (0 s/d 3000 RPM)
        speed_row.addWidget(QLabel("Preset:"))

        btn_p0 = QPushButton("🛑 0")
        btn_p0.setStyleSheet("background-color: #212529; color: #adb5bd; font-weight: bold; font-size: 10px; padding: 5px 6px; border-radius: 4px; border: 1px solid #495057;")
        btn_p0.setToolTip("Motor diam (0 RPM)")
        btn_p0.clicked.connect(lambda: self.set_speed_rpm(0))
        speed_row.addWidget(btn_p0)

        btn_p30 = QPushButton("🛡️ 30 (Aman)")
        btn_p30.setStyleSheet("background-color: #17323f; color: #80d8ff; font-weight: bold; font-size: 10px; padding: 5px 6px; border-radius: 4px; border: 1px solid #0091ea;")
        btn_p30.setToolTip("Kecepatan aman tanpa mounting (30 RPM)")
        btn_p30.clicked.connect(lambda: self.set_speed_rpm(30))
        speed_row.addWidget(btn_p30)

        btn_p100 = QPushButton("🏃 100")
        btn_p100.setStyleSheet("background-color: #2b2b1a; color: #ffd54f; font-weight: bold; font-size: 10px; padding: 5px 6px; border-radius: 4px; border: 1px solid #fbc02d;")
        btn_p100.setToolTip("Kecepatan sedang (100 RPM)")
        btn_p100.clicked.connect(lambda: self.set_speed_rpm(100))
        speed_row.addWidget(btn_p100)

        btn_p500 = QPushButton("⚡ 500")
        btn_p500.setStyleSheet("background-color: #3e2723; color: #ffb74d; font-weight: bold; font-size: 10px; padding: 5px 6px; border-radius: 4px; border: 1px solid #ff9800;")
        btn_p500.setToolTip("Kecepatan 500 RPM")
        btn_p500.clicked.connect(lambda: self.set_speed_rpm(500))
        speed_row.addWidget(btn_p500)

        btn_p1500 = QPushButton("🚀 1500")
        btn_p1500.setStyleSheet("background-color: #381a24; color: #ff80ab; font-weight: bold; font-size: 10px; padding: 5px 6px; border-radius: 4px; border: 1px solid #c2185b;")
        btn_p1500.setToolTip("Kecepatan tinggi (1500 RPM) - Pastikan motor difiksasi")
        btn_p1500.clicked.connect(lambda: self.set_speed_rpm(1500))
        speed_row.addWidget(btn_p1500)

        btn_p3000 = QPushButton("🔥 3000 (Maks)")
        btn_p3000.setStyleSheet("background-color: #4a1515; color: #ff5252; font-weight: bold; font-size: 10px; padding: 5px 6px; border-radius: 4px; border: 1px solid #d50000;")
        btn_p3000.setToolTip("Kecepatan maksimum servo Lichuan (3000 RPM)")
        btn_p3000.clicked.connect(lambda: self.set_speed_rpm(3000))
        speed_row.addWidget(btn_p3000)

        master_box.addLayout(speed_row)

        # Baris 2: Kontrol Serentak Semua 3 Motor (Hold-to-Jog Bersama) & Tombol Global
        action_row = QHBoxLayout()
        action_row.setSpacing(8)

        lbl_all = QLabel("KONTROL SERENTAK (3 MOTOR):")
        lbl_all.setStyleSheet("color: #a0aec0; font-weight: bold; font-size: 11px;")
        action_row.addWidget(lbl_all)

        # Mundur Semua
        self.btn_all_ccw = QPushButton("◀◀ MUNDUR SEMUA (Hold)")
        self.btn_all_ccw.setToolTip("Tekan terus untuk memutar mundur ketiga motor secara serempak. Lepas untuk berhenti.")
        self.btn_all_ccw.setStyleSheet("""
            QPushButton {
                background-color: #5c1d38; color: #ffffff; font-weight: bold; font-size: 11px; padding: 8px 14px; border-radius: 5px; border: 1px solid #ad1457;
            }
            QPushButton:hover { background-color: #7b1fa2; }
            QPushButton:pressed { background-color: #d81b60; }
        """)
        self.btn_all_ccw.pressed.connect(self._on_all_ccw_pressed)
        self.btn_all_ccw.released.connect(self._on_all_jog_released)
        action_row.addWidget(self.btn_all_ccw)

        # Maju Semua
        self.btn_all_cw = QPushButton("MAJU SEMUA (Hold) ▶▶")
        self.btn_all_cw.setToolTip("Tekan terus untuk memutar maju ketiga motor secara serempak. Lepas untuk berhenti.")
        self.btn_all_cw.setStyleSheet("""
            QPushButton {
                background-color: #1b5e20; color: #ffffff; font-weight: bold; font-size: 11px; padding: 8px 14px; border-radius: 5px; border: 1px solid #388e3c;
            }
            QPushButton:hover { background-color: #2e7d32; }
            QPushButton:pressed { background-color: #4caf50; }
        """)
        self.btn_all_cw.pressed.connect(self._on_all_cw_pressed)
        self.btn_all_cw.released.connect(self._on_all_jog_released)
        action_row.addWidget(self.btn_all_cw)

        # Reset Fault
        self.btn_fault_reset = QPushButton("🔄 RESET FAULT")
        self.btn_fault_reset.setToolTip("Kirim sinyal CiA402 Fault Reset (Bit 7) ke semua servo.")
        self.btn_fault_reset.setStyleSheet("""
            QPushButton {
                background-color: #e65100; color: #ffffff; font-weight: bold; font-size: 11px; padding: 8px 12px; border-radius: 5px; border: none;
            }
            QPushButton:hover { background-color: #f57c00; }
        """)
        self.btn_fault_reset.clicked.connect(self._on_fault_reset)
        action_row.addWidget(self.btn_fault_reset)

        # Tare Semua
        self.btn_tare_all = QPushButton("🎯 SET NOL SEMUA")
        self.btn_tare_all.setStyleSheet("""
            QPushButton {
                background-color: #2c344d; color: #ffffff; font-weight: bold; font-size: 11px; padding: 8px 12px; border-radius: 5px; border: none;
            }
            QPushButton:hover { background-color: #3b4668; }
        """)
        self.btn_tare_all.clicked.connect(self._tare_zero_all)
        action_row.addWidget(self.btn_tare_all)

        action_row.addStretch()

        # Stop Darurat
        self.btn_stop = QPushButton("🛑 STOP DARURAT (HALT)")
        self.btn_stop.setStyleSheet("""
            QPushButton {
                background-color: #b71c1c; color: white; font-weight: bold; font-size: 12px; padding: 8px 18px; border-radius: 5px; border: 1px solid #ff1744;
            }
            QPushButton:hover { background-color: #c62828; }
            QPushButton:pressed { background-color: #ff1744; }
        """)
        self.btn_stop.clicked.connect(self._emergency_stop)
        action_row.addWidget(self.btn_stop)

        master_box.addLayout(action_row)
        main_layout.addWidget(master_frame)

        # ── 5. Bottom Connection & Mode Bar ──
        conn_frame = QFrame()
        conn_frame.setStyleSheet("background-color: #121522; border: 1px solid #1f2538; border-radius: 8px;")
        conn_box = QHBoxLayout(conn_frame)
        conn_box.setContentsMargins(12, 8, 12, 8)
        conn_box.setSpacing(8)

        conn_box.addWidget(QLabel("NIC Interface:"))
        self.txt_iface = QLineEdit("enp2s0")
        self.txt_iface.setFixedWidth(80)
        self.txt_iface.setStyleSheet("background-color: #1a1e2d; color: #ffffff; font-weight: bold; padding: 5px; border-radius: 4px; border: 1px solid #333d5a;")
        conn_box.addWidget(self.txt_iface)

        # Tombol Mulai Kontrol (Online)
        self.btn_control_online = QPushButton("🎮 MULAI KONTROL (ONLINE)")
        self.btn_control_online.setToolTip("Hubungkan ke EtherCAT dan aktifkan kontrol motor 3 servo (PP Mode)")
        self.btn_control_online.setStyleSheet("""
            QPushButton {
                background-color: #0077b6; color: white; font-weight: bold; font-size: 11px; padding: 7px 14px; border-radius: 5px; border: none;
            }
            QPushButton:hover { background-color: #0096c7; }
        """)
        self.btn_control_online.clicked.connect(self._start_interactive_control)
        conn_box.addWidget(self.btn_control_online)

        # Tombol Monitor Saja
        self.btn_monitor = QPushButton("👁️ HANYA MONITOR")
        self.btn_monitor.setToolTip("Baca encoder 3 servo secara pasif tanpa enable motor")
        self.btn_monitor.setStyleSheet("""
            QPushButton {
                background-color: #263238; color: #b0bec5; font-weight: bold; font-size: 11px; padding: 7px 12px; border-radius: 5px; border: none;
            }
            QPushButton:hover { background-color: #37474f; color: #ffffff; }
        """)
        self.btn_monitor.clicked.connect(self._toggle_monitor)
        conn_box.addWidget(self.btn_monitor)

        # Tombol Tarian 3 Servo
        self.btn_dance = QPushButton("💃 TARIAN 3 SERVO")
        self.btn_dance.setStyleSheet("""
            QPushButton {
                background-color: #4a148c; color: #ea80fc; font-weight: bold; font-size: 11px; padding: 7px 12px; border-radius: 5px; border: none;
            }
            QPushButton:hover { background-color: #6a1b9a; }
        """)
        self.btn_dance.clicked.connect(self._run_dance)
        conn_box.addWidget(self.btn_dance)

        # Tombol Disconnect
        self.btn_disconnect = QPushButton("⏹ PUTUSKAN")
        self.btn_disconnect.setStyleSheet("""
            QPushButton {
                background-color: #212536; color: #90a4ae; font-weight: bold; font-size: 11px; padding: 7px 12px; border-radius: 5px; border: none;
            }
            QPushButton:hover { background-color: #37474f; color: #ffffff; }
        """)
        self.btn_disconnect.clicked.connect(self._stop_backend)
        conn_box.addWidget(self.btn_disconnect)

        conn_box.addStretch()

        # Checkbox Mode Demo / Simulasi
        self.chk_demo = QCheckBox("Mode Simulasi (Demo Interaktif)")
        self.chk_demo.setStyleSheet("color: #a0aec0; font-size: 11px; font-weight: bold;")
        self.chk_demo.toggled.connect(self._toggle_demo_mode)
        conn_box.addWidget(self.chk_demo)

        main_layout.addWidget(conn_frame)

        # ── 6. Dance Phase Bar (Optional) ──
        self._dance_bar_frame = QFrame()
        self._dance_bar_frame.setStyleSheet("background-color: #110e1a; border: 1px solid #4a2070; border-radius: 6px;")
        self._dance_bar_frame.setVisible(False)
        dance_bar_box = QHBoxLayout(self._dance_bar_frame)
        dance_bar_box.setContentsMargins(12, 4, 12, 4)
        dance_bar_box.setSpacing(8)

        lbl_dp = QLabel("🎭 TARIAN 3 SERVO:")
        lbl_dp.setStyleSheet("color: #ce93d8; font-weight: bold; font-size: 11px; border: none;")
        dance_bar_box.addWidget(lbl_dp)

        self._lbl_dance_phase = QLabel("Fase 0/8")
        self._lbl_dance_phase.setStyleSheet("color: #ea80fc; font-size: 11px; font-weight: bold; border: none; min-width: 60px;")
        dance_bar_box.addWidget(self._lbl_dance_phase)

        self._lbl_dance_name = QLabel("")
        self._lbl_dance_name.setStyleSheet("color: #f3e5f5; font-size: 10px; border: none;")
        dance_bar_box.addWidget(self._lbl_dance_name, 1)

        self._dance_dots = []
        for i in range(8):
            dot = QLabel("●")
            dot.setStyleSheet("color: #3d2050; font-size: 14px; border: none;")
            dance_bar_box.addWidget(dot)
            self._dance_dots.append(dot)

        main_layout.addWidget(self._dance_bar_frame)

        # ── 7. Terminal Console ──
        self.console = QTextEdit()
        self.console.setMaximumHeight(85)
        self.console.setReadOnly(True)
        self.console.setStyleSheet("""
            QTextEdit {
                background-color: #080a10;
                color: #8c9ac4;
                font-family: Monospace;
                font-size: 10px;
                border: 1px solid #1a2030;
                border-radius: 6px;
                padding: 4px;
            }
        """)
        self.console.append("Siap. Pilih kecepatan RPM (default 25 RPM), lalu klik 'MULAI KONTROL (ONLINE)' atau 'Mode Simulasi' untuk menggerakkan 3 motor.")
        main_layout.addWidget(self.console)

    # ─────────────────────────────────────────────────────────────────────────
    # Kecepatan & Keselamatan
    # ─────────────────────────────────────────────────────────────────────────
    def set_speed_rpm(self, rpm):
        self.slider_rpm.setValue(rpm)

    def _on_slider_changed(self, val):
        self.current_rpm = val
        self.spin_rpm.blockSignals(True)
        self.spin_rpm.setValue(val)
        self.spin_rpm.blockSignals(False)
        self._update_safety_badge()
        if self.process and self.process.state() != QProcess.ProcessState.NotRunning:
            self._send_command(f"SET_RPM {self.current_rpm}\n")

    def _on_spin_changed(self, val):
        self.current_rpm = val
        self.slider_rpm.blockSignals(True)
        self.slider_rpm.setValue(val)
        self.slider_rpm.blockSignals(False)
        self._update_safety_badge()
        if self.process and self.process.state() != QProcess.ProcessState.NotRunning:
            self._send_command(f"SET_RPM {self.current_rpm}\n")

    def _update_safety_badge(self):
        if self.current_rpm == 0:
            self.lbl_safety_mode.setText("🛑 0 RPM: STANDSTILL (MOTOR DIAM / TIDAK BERPUTAR)")
            self.lbl_safety_mode.setStyleSheet("background-color: #212529; color: #adb5bd; font-size: 10px; font-weight: bold; padding: 5px 10px; border-radius: 4px; border: 1px solid #495057;")
        elif self.current_rpm <= 60:
            self.lbl_safety_mode.setText(f"🛡️ MODE AMAN: {self.current_rpm} RPM (PELAN / AMAN TANPA MOUNTING)")
            self.lbl_safety_mode.setStyleSheet("background-color: #1b3a24; color: #69f0ae; font-size: 10px; font-weight: bold; padding: 5px 10px; border-radius: 4px; border: 1px solid #2e7d32;")
        elif self.current_rpm <= 500:
            self.lbl_safety_mode.setText(f"⚡ KECEPATAN SEDANG: {self.current_rpm} RPM (PASTIKAN MOTOR DITAHAN)")
            self.lbl_safety_mode.setStyleSheet("background-color: #3e2723; color: #ffb74d; font-size: 10px; font-weight: bold; padding: 5px 10px; border-radius: 4px; border: 1px solid #ff9800;")
        else:
            self.lbl_safety_mode.setText(f"⚠️ KECEPATAN TINGGI: {self.current_rpm} RPM! (BAHAYA JIKA BELUM DIBUATKAN MOUNTING KUAT!)")
            self.lbl_safety_mode.setStyleSheet("background-color: #5c1d1d; color: #ff8a80; font-size: 10px; font-weight: bold; padding: 5px 10px; border-radius: 4px; border: 1px solid #d50000;")

    # ─────────────────────────────────────────────────────────────────────────
    # Logika Hold-to-Jog
    # ─────────────────────────────────────────────────────────────────────────
    def start_jog(self, slave_id, direction):
        """Dipanggil saat tombol jog ditekan (Hold-to-Jog)."""
        if self.current_rpm == 0:
            self.console.append("[INFO] 🛑 RPM = 0 (Motor Diam). Geser slider RPM (misal 30 - 1000 RPM) untuk memutar motor.")
            return

        if self.is_demo:
            self.sim_active_jogs[slave_id] = 1 if direction == '+' else -1
            return

        cmd = f"JOG {slave_id} {direction} {self.current_rpm}\n"
        self._send_command(cmd)

    def stop_jog(self, slave_id):
        """Dipanggil saat tombol jog dilepas."""
        if self.is_demo:
            if slave_id == 0:
                self.sim_active_jogs.clear()
            else:
                self.sim_active_jogs.pop(slave_id, None)
            return

        cmd = f"HALT {slave_id}\n"
        self._send_command(cmd)

    def step_servo(self, slave_id, delta_pulses):
        """Putar motor sejumlah pulsa tertentu (misal 1 rotasi = 131.072 pulsa)."""
        if self.current_rpm == 0:
            self.console.append("[INFO] 🛑 RPM = 0 (Motor Diam). Geser slider RPM (misal 30 - 1000 RPM) untuk memutar motor.")
            return

        if self.is_demo:
            self.sim_positions[slave_id] += delta_pulses
            self.console.append(f"[STEP DEMO] Motor {slave_id} melangkah {delta_pulses} pulsa.")
            return

        cmd = f"STEP {slave_id} {delta_pulses} {self.current_rpm}\n"
        self._send_command(cmd)

    def _on_all_cw_pressed(self):
        if self.current_rpm == 0:
            self.console.append("[INFO] 🛑 RPM = 0 (Motor Diam). Geser slider RPM (misal 30 - 1000 RPM) untuk memutar ketiga motor.")
            return
        self.card1.set_motion_status(f"▶ MAJU @ {self.current_rpm} RPM", active=True, is_cw=True)
        self.card2.set_motion_status(f"▶ MAJU @ {self.current_rpm} RPM", active=True, is_cw=True)
        self.card3.set_motion_status(f"▶ MAJU @ {self.current_rpm} RPM", active=True, is_cw=True)
        self.start_jog(0, "+")

    def _on_all_ccw_pressed(self):
        if self.current_rpm == 0:
            self.console.append("[INFO] 🛑 RPM = 0 (Motor Diam). Geser slider RPM (misal 30 - 1000 RPM) untuk memutar ketiga motor.")
            return
        self.card1.set_motion_status(f"◀ MUNDUR @ {self.current_rpm} RPM", active=True, is_cw=False)
        self.card2.set_motion_status(f"◀ MUNDUR @ {self.current_rpm} RPM", active=True, is_cw=False)
        self.card3.set_motion_status(f"◀ MUNDUR @ {self.current_rpm} RPM", active=True, is_cw=False)
        self.start_jog(0, "-")

    def _on_all_jog_released(self):
        self.card1.set_motion_status("", active=False)
        self.card2.set_motion_status("", active=False)
        self.card3.set_motion_status("", active=False)
        self.stop_jog(0)

    def _emergency_stop(self):
        self.console.append("[STOP] 🛑 MENGIRIM PERINTAH STOP DARURAT KE SEMUA MOTOR!")
        if self.is_demo:
            self.sim_active_jogs.clear()
            self.card1.set_motion_status("", active=False)
            self.card2.set_motion_status("", active=False)
            self.card3.set_motion_status("", active=False)
            return

        self._send_command("STOP\n")
        self.card1.set_motion_status("", active=False)
        self.card2.set_motion_status("", active=False)
        self.card3.set_motion_status("", active=False)

    def _on_fault_reset(self):
        self.console.append("[RESET] 🔄 Mengirim sinyal Fault Reset CiA402...")
        if self.is_demo:
            self.card1.badge_fault.setStyleSheet("background-color: #1c2030; color: #555e7e;")
            self.card2.badge_fault.setStyleSheet("background-color: #1c2030; color: #555e7e;")
            self.card3.badge_fault.setStyleSheet("background-color: #1c2030; color: #555e7e;")
            return
        self._send_command("FAULT_RESET\n")

    def _tare_zero_all(self):
        self.card1.zero_tare(self.last_raw1)
        self.card2.zero_tare(self.last_raw2)
        self.card3.zero_tare(self.last_raw3)
        self.console.append(f"[TARE] Titik nol ketiga motor disimpan: S1={self.last_raw1}, S2={self.last_raw2}, S3={self.last_raw3}")

    def _send_command(self, cmd_str):
        if not cmd_str.endswith("\n"):
            cmd_str += "\n"
        self.console.append(f"➔ {cmd_str.strip()}")
        if self.process and self.process.state() != QProcess.ProcessState.NotRunning:
            self.process.write(cmd_str.encode("utf-8"))
            self.process.waitForBytesWritten(50)
        else:
            if not self.is_demo:
                self.console.append("[WARN] Backend EtherCAT belum online. Klik 'Mulai Kontrol Interaktif' terlebih dahulu.")

    # ─────────────────────────────────────────────────────────────────────────
    # Logika Eksekusi Backend EtherCAT
    # ─────────────────────────────────────────────────────────────────────────
    def _start_backend(self, mode_flag, extra_args=None):
        self._stop_backend()
        iface = self.txt_iface.text().strip() or "enp2s0"
        bin_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "build", "ethercat_servo")
        if not os.path.exists(bin_path):
            self.console.append(f"[ERROR] Executable {bin_path} belum di-build! Jalankan 'cmake --build build'.")
            return

        if os.geteuid() == 0:
            cmd = [bin_path, iface, mode_flag]
        else:
            cmd = ["sudo", bin_path, iface, mode_flag]
        if extra_args:
            cmd.extend(extra_args)
        self.console.append(f"[EXEC] {' '.join(cmd)}")

        self.process = QProcess(self)
        self.process.readyReadStandardOutput.connect(self._on_process_output)
        self.process.readyReadStandardError.connect(self._on_process_error)
        self.process.finished.connect(self._on_process_finished)

        self.process.start(cmd[0], cmd[1:])
        self.lbl_bus_status.setText("STATUS: MENGHUBUNGKAN...")
        self.lbl_bus_status.setStyleSheet("background-color: #bf360c; color: #ffffff; font-weight: bold; padding: 5px 10px; border-radius: 4px;")

    def _start_interactive_control(self):
        self.chk_demo.setChecked(False)
        self._start_backend("--control", extra_args=["--force-brake-confirmed", "--rpm", str(self.current_rpm)])

    def _toggle_monitor(self):
        if self.process and self.process.state() != QProcess.ProcessState.NotRunning:
            self._stop_backend()
        else:
            self.chk_demo.setChecked(False)
            self._start_backend("--monitor")

    def _run_dance(self):
        self.chk_demo.setChecked(False)
        self._dance_phase = 0
        self._update_dance_bar(0, "Memulai Tarian 3 Servo...")
        self._dance_bar_frame.setVisible(True)
        self._start_backend("--dance", extra_args=["--force-brake-confirmed"])

    def _stop_backend(self):
        if self.process:
            self._send_command("QUIT\n")
            self.console.append("[INFO] Menghentikan proses EtherCAT...")
            self.process.terminate()
            if not self.process.waitForFinished(1000):
                self.process.kill()
            self.process = None

        self._dance_bar_frame.setVisible(False)
        self.lbl_bus_status.setText("STATUS: STANDBY")
        self.lbl_bus_status.setStyleSheet("background-color: #191f32; color: #8da2d8; font-weight: bold; padding: 5px 10px; border-radius: 4px; border: 1px solid #2c3654;")
        self.lbl_wkc.setText("WKC: - / -")

    def _update_dance_bar(self, phase: int, name: str = ""):
        self._dance_phase = phase
        self._lbl_dance_phase.setText(f"Fase {phase}/{self._dance_total}")
        self._lbl_dance_name.setText(name)
        colors = {
            'done':    "color: #69f0ae; font-size: 14px; border: none;",
            'current': "color: #ea80fc; font-size: 18px; border: none;",
            'future':  "color: #3d2050; font-size: 14px; border: none;",
        }
        for i, dot in enumerate(self._dance_dots):
            idx = i + 1
            if idx < phase:
                dot.setStyleSheet(colors['done'])
            elif idx == phase:
                dot.setStyleSheet(colors['current'])
            else:
                dot.setStyleSheet(colors['future'])

    def _on_process_output(self):
        if not self.process:
            return
        raw = self.process.readAllStandardOutput().data().decode("utf-8", errors="replace")
        for line in raw.splitlines():
            line = line.strip()
            if not line:
                continue

            if "READY_FOR_CONTROL" in line:
                self.lbl_bus_status.setText("STATUS: KONTROL ONLINE (3 MOTOR AKTIF)")
                self.lbl_bus_status.setStyleSheet("background-color: #1b5e20; color: #69f0ae; font-weight: bold; padding: 5px 10px; border-radius: 4px;")
                self.console.append("✓ 3 Servo Enabled dalam PP mode. Siap untuk Hold-to-Jog!")
                continue

            m_dance = re.search(r"Fase\s+(\d+)/(\d+)\s+(.+)", line)
            if m_dance and "▶" in line:
                ph = int(m_dance.group(1))
                name = m_dance.group(3).strip()
                self._update_dance_bar(ph, name)
                self.lbl_bus_status.setText(f"TARIAN: FASE {ph}/{self._dance_total}")
                self.lbl_bus_status.setStyleSheet("background-color: #4a148c; color: #ea80fc; font-weight: bold; padding: 5px 10px; border-radius: 4px;")

            if "TARIAN SELESAI" in line:
                self._update_dance_bar(self._dance_total, "✓ SELESAI!")
                self.lbl_bus_status.setText("TARIAN SELESAI ✓")
                self.lbl_bus_status.setStyleSheet("background-color: #1b5e20; color: #69f0ae; font-weight: bold; padding: 5px 10px; border-radius: 4px;")

            # Telemetry parser 3 Servo: pos1, sw1, torq1, pos2, sw2, torq2, pos3, sw3, torq3, wkc
            m = re.search(
                r"TELEMETRY:\s+pos1=(-?\d+)\s+sw1=([0-9A-Fa-f]+)\s+torq1=(-?\d+)"
                r"\s+pos2=(-?\d+)\s+sw2=([0-9A-Fa-f]+)\s+torq2=(-?\d+)"
                r"(?:\s+pos3=(-?\d+)\s+sw3=([0-9A-Fa-f]+)\s+torq3=(-?\d+))?\s+wkc=(\d+)",
                line)
            if m:
                p1  = int(m.group(1))
                sw1 = int(m.group(2), 16)
                t1  = int(m.group(3))
                p2  = int(m.group(4))
                sw2 = int(m.group(5), 16)
                t2  = int(m.group(6))
                p3  = int(m.group(7)) if m.group(7) is not None else 0
                sw3 = int(m.group(8), 16) if m.group(8) is not None else 0
                t3  = int(m.group(9)) if m.group(9) is not None else 0
                wkc = int(m.group(10))

                self.last_raw1 = p1
                self.last_raw2 = p2
                self.last_raw3 = p3
                self.card1.update_telemetry(p1, sw1, t1)
                self.card2.update_telemetry(p2, sw2, t2)
                self.card3.update_telemetry(p3, sw3, t3)
                self.waveform.add_points(p1 - self.card1.offset,
                                         p2 - self.card2.offset,
                                         p3 - self.card3.offset)

                if "KONTROL ONLINE" not in self.lbl_bus_status.text():
                    self.lbl_bus_status.setText("STATUS: OPERATIONAL (OP)")
                    self.lbl_bus_status.setStyleSheet("background-color: #1b5e20; color: #69f0ae; font-weight: bold; padding: 5px 10px; border-radius: 4px;")
                self.lbl_wkc.setText(f"WKC: {wkc} OK")
                continue

            # Non-telemetry lines to console
            self.console.append(line)

    def _on_process_error(self):
        if not self.process:
            return
        err = self.process.readAllStandardError().data().decode("utf-8", errors="replace")
        self.console.append(f"[STDERR] {err.strip()}")
        if "kata sandi" in err.lower() or "password" in err.lower():
            self.console.append("❌ [SUDO ERROR] Sudo memerlukan kata sandi! Silakan jalankan GUI di terminal dengan: sudo python3 servo_gui.py")

    def _on_process_finished(self, code, status):
        self.console.append(f"[INFO] Backend selesai (Exit code {code}).")
        self._stop_backend()

    # ─────────────────────────────────────────────────────────────────────────
    # Mode Demo / Simulasi Interaktif
    # ─────────────────────────────────────────────────────────────────────────
    def _toggle_demo_mode(self, enabled):
        self.is_demo = enabled
        if enabled:
            self._stop_backend()
            self.console.append("[DEMO] Mode simulasi diaktifkan: Anda dapat menguji tombol Hold-to-Jog, slider RPM, dan osiloskop langsung di layar!")
            self.lbl_bus_status.setText("STATUS: SIMULASI (DEMO)")
            self.lbl_bus_status.setStyleSheet("background-color: #4a148c; color: #ea80fc; font-weight: bold; padding: 5px 10px; border-radius: 4px;")
            self.lbl_wkc.setText("WKC: 9/9 (SIM)")
            self.sim_timer.start(20)  # 50 Hz update
        else:
            self.sim_timer.stop()
            self.sim_active_jogs.clear()
            self.lbl_bus_status.setText("STATUS: STANDBY")
            self.lbl_bus_status.setStyleSheet("background-color: #191f32; color: #8da2d8; font-weight: bold; padding: 5px 10px; border-radius: 4px; border: 1px solid #2c3654;")
            self.lbl_wkc.setText("WKC: - / -")

    def _update_sim_step(self):
        dt = 0.02  # 20ms
        pulses_per_sec = (self.current_rpm * PULSES_PER_REV) / 60.0

        for slv in [1, 2, 3]:
            direction = 0
            if slv in self.sim_active_jogs:
                direction = self.sim_active_jogs[slv]
            elif 0 in self.sim_active_jogs:
                direction = self.sim_active_jogs[0]

            if direction != 0:
                self.sim_positions[slv] += direction * pulses_per_sec * dt

        p1 = int(self.sim_positions[1])
        p2 = int(self.sim_positions[2])
        p3 = int(self.sim_positions[3])

        self.last_raw1 = p1
        self.last_raw2 = p2
        self.last_raw3 = p3

        sw = 0x0637
        self.card1.update_telemetry(p1, sw, torq_val=int(12 if 1 in self.sim_active_jogs or 0 in self.sim_active_jogs else 0))
        self.card2.update_telemetry(p2, sw, torq_val=int(12 if 2 in self.sim_active_jogs or 0 in self.sim_active_jogs else 0))
        self.card3.update_telemetry(p3, sw, torq_val=int(12 if 3 in self.sim_active_jogs or 0 in self.sim_active_jogs else 0))

        self.waveform.add_points(p1 - self.card1.offset,
                                 p2 - self.card2.offset,
                                 p3 - self.card3.offset)


# ─────────────────────────────────────────────────────────────────────────────
# Entry Point
# ─────────────────────────────────────────────────────────────────────────────
def main():
    app = QApplication(sys.argv)
    app.setStyle("Fusion")

    window = TripleServoWindow()
    window.show()
    sys.exit(app.exec_() if hasattr(app, "exec_") else app.exec())


if __name__ == "__main__":
    main()
