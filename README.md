#  Smart IoT-Based Parking & Traffic Management System  

An IoT-based solution that combines **smart parking** and **adaptive traffic light control** using **ESP32**, sensors, and cloud integration.  
The system was simulated on **Wokwi**, connected to **HiveMQ Cloud (MQTT)**, **Supabase Database**, and visualized with a **Flutter mobile app**.  

---

##  Features  

###  Parking System  
- 4 parking slots monitored with **IR break beam sensors** (digital inputs).  
- Automated entrance gate using **IR sensor + Servo motor**.  
- **LCD display** shows real-time parking availability (Available / Full).  

###  Traffic Management System  
- 4-street intersection with traffic lights (digital outputs).  
- **Ultrasonic sensors** detect congestion (distance threshold = 7cm).  
- Adaptive timing: **10s normal → 15s if congestion detected**.  
- Prioritization for streets with higher traffic density.  

###  IoT Integration  
- **ESP32** as the central controller.  
- Data published to **HiveMQ Cloud (MQTT)**.  
- Stored in **Supabase Database** for persistence.  
- **Flutter app** displays parking availability & traffic status.  

---

##  Tech Stack  
- **Hardware**: ESP32, IR sensors, Ultrasonic sensors, Servo motor, LCD (I2C).  
- **Cloud**: HiveMQ Cloud (MQTT), Supabase Database.  
- **Software**: Wokwi Simulator, Flutter Mobile App.  
- **Programming**: C++ (Arduino), Dart (Flutter).  

---

##  Results  
- Real-time parking monitoring and automated gate control.  
- Dynamic traffic signal timing reduces congestion.  
- Seamless simulation with Wokwi and integration with cloud + mobile app.  

---

##  Future Work  
- Real hardware deployment in a small-scale prototype.  
- AI/ML-based traffic prediction models.  
- Mobile notifications for drivers.  
- Integration with navigation apps (Google Maps, Waze).  

---

##  Authors  
Developed by students of **Alexandria University – Faculty of Computers and Data Science (Class of 2027)**. 
Salma Abd El-Aziz
Fajr Abu Bakr
Arwa Salem
Mariam Badry

