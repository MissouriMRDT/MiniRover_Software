const JOYSTICK_CLIP_RANGE = 0.2;
const GAMMA = 1.5;
const JOINT_SPEED = 0.02; // range/s
const IK_SPEED = 4; // mm/s
const COMMAND_INTERVAL = 5; // s

function lerp(x, x0, x1, y0, y1) {
    return (y0 * (x1 - x) + y1 * (x - x0)) / (x1 - x0);
}

function clamp(x, x0, x1) {
    return x < x0 ? x0 : (x > x1 ? x1 : x);
}

function lerpClamp(x, x0, x1, y1, y2) {
    return clamp(lerp(x, x0, x1, y1, y2), y1, y2);
}

function arcadeDrive(rotate, drive) {
    // Convert to polar.
    let r = Math.hypot(rotate, drive);
    let t = Math.atan2(drive, rotate);
    // Rotate by 45 degrees.
    t -= Math.PI / 4;
    // Convert to cartesian, rescale and clamp.
    return [
        r * Math.cos(t) * 2 / Math.SQRT2,
        r * Math.sin(t) * 2 / Math.SQRT2
    ];
}

class Joystick {
    dom;
    x;
    y;
    active;
    innerX;
    innerY;
    constructor(id) {
        this.dom = document.getElementById(id);
        this.x = 0;
        this.y = 0;
        this.dom.classList.add("joystick");

        this.dom.addEventListener("touchmove", this.onTouchMove.bind(this), false);
        this.dom.addEventListener("touchend", this.onTouchEnd.bind(this), false);
        this.dom.addEventListener("mousemove", this.onMouseMove.bind(this), false);
        this.dom.addEventListener("mouseleave", this.onMouseLeave.bind(this), false);
        this.innerX = document.createElement("div");
        this.innerX.className = "joystick-x";
        this.dom.appendChild(this.innerX);
        this.innerY = document.createElement("div");
        this.innerY.className = "joystick-y";
        this.dom.appendChild(this.innerY);
    }
    onTouchMove(event) {
        let bound = this.dom.getBoundingClientRect();
        let x = 0;
        let y = 0;
        let active = false;
        for (const touch of event.touches) {
            if (touch.clientX > bound.left - bound.width * JOYSTICK_CLIP_RANGE
                && touch.clientX < bound.right + bound.width * JOYSTICK_CLIP_RANGE
                && touch.clientY > bound.top - bound.height * JOYSTICK_CLIP_RANGE
                && touch.clientY < bound.bottom + bound.height * JOYSTICK_CLIP_RANGE) {
                x = lerpClamp(touch.clientX, bound.left, bound.right, -1, 1);
                y = lerpClamp(touch.clientY, bound.top, bound.bottom, -1, 1);
                active = true;
                break;
            }
        }
        this.x = x;
        this.y = y;
        this.innerX.style.left = `${this.x * 50 + 50}%`;
        this.innerY.style.top = `${this.y * 50 + 50}%`;
        this.active = active;
    }
    onTouchEnd(event) {
        this.x = 0;
        this.y = 0;
        this.innerX.style.left = `${this.x * 50 + 50}%`;
        this.innerY.style.top = `${this.y * 50 + 50}%`;
        this.active = false;
    }
    onMouseMove(event) {
        let bound = this.dom.getBoundingClientRect();
        if (event.clientX > bound.left - bound.width * JOYSTICK_CLIP_RANGE
            && event.clientX < bound.right + bound.width * JOYSTICK_CLIP_RANGE
            && event.clientY > bound.top - bound.height * JOYSTICK_CLIP_RANGE
            && event.clientY < bound.bottom + bound.height * JOYSTICK_CLIP_RANGE) {
            this.x = lerpClamp(event.clientX, bound.left, bound.right, -1, 1);
            this.y = lerpClamp(event.clientY, bound.top, bound.bottom, -1, 1);
            this.active = true;
        } else {
            this.x = 0;
            this.y = 0;
            this.active = false;
        }
        this.innerX.style.left = `${this.x * 50 + 50}%`;
        this.innerY.style.top = `${this.y * 50 + 50}%`;
    }
    onMouseLeave(event) {
        this.x = 0;
        this.y = 0;
        this.innerX.style.left = `${this.x * 50 + 50}%`;
        this.innerY.style.top = `${this.y * 50 + 50}%`;
        this.active = false;
    }
}

class Bar {
    dom;
    top;
    bottom;
    label;
    value;
    min;
    nomMin;
    nomMax;
    nomMinP;
    nomMaxP;
    max;
    format;
    constructor(id, min, nomMin, nomMax, max, format) {
        this.value = min;
        this.min = min;
        this.nomMin = nomMin;
        this.nomMax = nomMax;
        this.max = max;
        this.nomMinP = this.p(this.nomMin);
        this.nomMaxP = this.p(this.nomMax);
        this.format = format;

        this.dom = document.getElementById(id);
        this.dom.style.gridArea = id;
        this.top = document.createElement("div");
        this.dom.appendChild(this.top);
        if (this.min !== this.nomMin || this.max !== this.nomMax) {
            this.bottom = document.createElement("div");
            this.dom.appendChild(this.bottom);
        } else {
            this.top.style.height = "100%";
        }
        this.label = document.createElement("p");
        this.label.style.gridArea = id;
        this.dom.parentElement.appendChild(this.label);
    }
    update(value) {
        this.value = value;
        let fill = Math.min(Math.max(this.p(this.value), 0), 100);
        if (this.bottom !== undefined) {
            this.bottom.style.backgroundImage = `linear-gradient(to right, #800 ${this.nomMinP}%, #080 ${this.nomMinP}% ${this.nomMaxP}%, #880 ${this.nomMaxP}% 100%)`;
        }
        if (this.value < this.nomMin) {
            this.top.style.backgroundImage = `linear-gradient(to right, #d00 ${fill}%, #000 0%)`;
        } else if (this.value < this.nomMax) {
            this.top.style.backgroundImage = `linear-gradient(to right, #0d0 ${fill}%, #000 0%)`;
        } else {
            this.top.style.backgroundImage = `linear-gradient(to right, #dd0 ${fill}%, #000 0%)`;
        }
        this.label.textContent = this.format[0].replaceAll("@", this.value.toFixed(this.format[2]).padStart(this.format[1] + this.format[2] + (this.format[2] > 0 ? 1 : 0), "0"));
    }
    p(x) {
        return (x - this.min) / (this.max - this.min) * 100;
    }
}

window.onload = () => {
    const socket = new WebSocket(`ws://192.168.4.1/ws`);
    socket.binaryType = "arraybuffer";
    socket.addEventListener("message", event => {
        // telemetry {
        //   float batteryVoltage;
        //   float batteryCurrent;
        //   float cell1;
        //   float cell2;
        //   float cell3;
        //   float cell4;
        //   int16_t l;
        //   int16_t r;
        //   uint16_t ax;
        //   uint16_t j1;
        //   uint16_t j2;
        //   uint16_t j3;
        //   uint16_t x;
        //   uint16_t y;
        //   uint16_t z;
        // }
        let data = new DataView(event.data);
        telemetry.bv.update(data.getFloat32(0, false));
        telemetry.ba.update(data.getFloat32(4, false));
        telemetry.c1.update(data.getFloat32(8, false));
        telemetry.c2.update(data.getFloat32(12, false));
        telemetry.c3.update(data.getFloat32(16, false));
        telemetry.c4.update(data.getFloat32(20, false));
        telemetry.dl.update(Math.abs(data.getInt16(24, false) / 0x7000 * 100));
        telemetry.dr.update(Math.abs(data.getInt16(26, false) / 0x7000 * 100));
        let ax = data.getUint16(28, false) / 0x10000;
        let aj1 = data.getUint16(30, false) / 0x10000;
        let aj2 = data.getUint16(32, false) / 0x10000;
        let aj3 = data.getUint16(34, false) / 0x10000;
        let aex = data.getUint16(36, false);
        let aey = data.getUint16(38, false);
        let aez = data.getUint16(40, false);

        telemetry.ax.update(ax * 100);
        telemetry.aj1.update(aj1 * 100);
        telemetry.aj2.update(aj2 * 100);
        telemetry.aj3.update(aj3 * 100);
        telemetry.aex.update(aex);
        telemetry.aey.update(aey);
        telemetry.aez.update(aez);

        // Set the non-controlled target values from telemetry.
        if (ik) {
            targetAngles.x = ax;
            targetAngles.j1 = j1;
            targetAngles.j2 = j2;
            targetAngles.j3 = j3;
        } else {
            targetIK.x = aex;
            targetIK.y = aey;
            targetIK.z = aez;
        }
    });

    let authorized = false;
    let targetAngles = {
        x: 0,
        j1: 0,
        j2: 0,
        j3: 0
    };
    let targetIK = {
        x: 0,
        y: 0,
        z: 0,
    };
    let ik = false;
    let lastTime = performance.now();
    let override = false;
    let locked = true;

    let drive = new Joystick("drive");
    let armLeft = new Joystick("arm-left");
    let armRight = new Joystick("arm-right");
    let lock = document.getElementById("lock");
    let on = document.getElementById("on");
    let off = document.getElementById("off");
    let display = document.getElementById("display");
    let overrideElements = Array.from(document.getElementsByClassName("override"));

    overrideElements.forEach(dom => dom.style.backgroundColor = "#880");
    overrideElements.forEach(dom => {
        dom.addEventListener("click", _ => {
            override = override ? false : authorized;
            overrideElements.forEach(dom => dom.style.backgroundColor = override ? "#dd0" : "#880");
        });
    });
    document.getElementById("telemetry").addEventListener("click", _ => document.body.requestFullscreen());
    lock.addEventListener("click", _ => {
        if (authorized) { authorized = false; }
        else {
            authorized = prompt("Password") === "nandgate";
        }
        if (authorized) {
            lock.children[0].style.display = "none"; lock.children[1].style.display = "initial";
        } else {
            lock.children[0].style.display = "initial"; lock.children[1].style.display = "none";
        }
    });
    lock.children[1].style.display = "none";
    on.addEventListener("click", _ => {
        if (authorized && socket.readyState === WebSocket.OPEN) {
            const buffer = new ArrayBuffer(1);
            new DataView(buffer).setUint8(0, 1);
            socket.send(buffer);
        }
    });
    off.addEventListener("click", _ => {
        if (socket.readyState === WebSocket.OPEN) {
            const buffer = new ArrayBuffer(1);
            new DataView(buffer).setUint8(0, 0);
            socket.send(buffer);
        }
    });

    let telemetry = {
        "bv": [0, 8, 12, 15, ["@V", 2, 1]],
        "ba": [0, 0, 10, 12, ["@A", 2, 1]],
        "c1": [0, 2, 3, 4, ["@V 1", 1, 1]],
        "c2": [0, 2, 3, 4, ["@V 2", 1, 1]],
        "c3": [0, 2, 3, 4, ["@V 3", 1, 1]],
        "c4": [0, 2, 3, 4, ["@V 4", 1, 1]],
        "dl": [0, 0, 100, 100, ["@% L", 3, 0]],
        "dr": [0, 0, 100, 100, ["@% R", 3, 0]],
        "ax": [0, 0, 100, 100, ["@% X", 3, 0]],
        "aj1": [0, 0, 100, 100, ["@% J1", 3, 0]],
        "aj2": [0, 0, 100, 100, ["@% J2", 3, 0]],
        "aj3": [0, 0, 100, 100, ["@% J3", 3, 0]],
        "aex": [0, 0, 500, 500, ["@mm X", 3, 0]],
        "aey": [0, 0, 500, 500, ["@mm Y", 3, 0]],
        "aez": [0, 0, 500, 500, ["@mm Z", 3, 0]],
    };
    Object.keys(telemetry).forEach(id => {
        telemetry[id] = new Bar(id, ...telemetry[id]);
    });

    setInterval(() => {
        /*
         *  // id: union
         *  // 0: off
         *  // 1: on
         *  // 2: drive [bool override, i16 left, i16 right]
         *  // 3: arm target angles [bool override, u16 x, u16 j1, u16 j2, u16 j3]
         *  // 4: arm IK [bool override, u16 x, u16 y, u16 z]
         *  typedef struct __attribute__((__packed__, scalar_storage_order("big-endian")))
         *  command {
         *  uint8_t id;
         *  union {
         *      struct __attribute__((__packed__, scalar_storage_order("big-endian"))) {
         *      bool override;
         *      int16_t left;
         *      int16_t right;
         *      } drive;
         *      struct __attribute__((__packed__, scalar_storage_order("big-endian"))) {
         *      bool override;
         *      uint16_t x;
         *      uint16_t j1;
         *      uint16_t j2;
         *      uint16_t j3;
         *      } armAngles;
         *      struct __attribute__((__packed__, scalar_storage_order("big-endian"))) {
         *      bool override;
         *      uint16_t x;
         *      uint16_t y;
         *      uint16_t z;
         *      } armIK;
         *  };
         *  } command;
         */
        let time = performance.now();
        let deltaT = (time - lastTime) * 1000;
        lastTime = time;

        let driveSpeed = 1; // TODO: change max drive speed.
        let driveX = drive.x;
        let driveY = drive.y;
        // Scale exponentially.
        driveX = driveX < 0 ? -Math.pow(-driveX, GAMMA) : Math.pow(driveX, GAMMA);
        driveY = driveY < 0 ? -Math.pow(-driveY, GAMMA) : Math.pow(driveY, GAMMA);
        let [leftSpeed, rightSpeed] = arcadeDrive(driveX, driveY);
        leftSpeed = clamp(leftSpeed, -1, 1, -driveSpeed, driveSpeed);
        rightSpeed = clamp(rightSpeed, -1, 1, -driveSpeed, driveSpeed);

        let buffer = new ArrayBuffer(10);
        let data = new DataView(buffer);
        data.setUint8(0, 2, false);
        data.setUint8(1, override, false);
        data.setInt16(2, leftSpeed * 0x7000, false);
        data.setInt16(4, rightSpeed * 0x7000, false);

        if (socket.readyState === WebSocket.OPEN) socket.send(buffer);

        if (ik) {
            targetAngles.x = clamp(targetAngles.x + armRight.x * IK_SPEED * deltaT, 0, 500);
            targetAngles.j1 = clamp(targetAngles.j1 + armRight.y * IK_SPEED * deltaT, 0, 500);
            targetAngles.j2 = clamp(targetAngles.j2 + armLeft.x * IK_SPEED * deltaT, 0, 500);
            targetAngles.j3 = clamp(targetAngles.j3 + armLeft.y * IK_SPEED * deltaT, 0, 500);

            data.setUint8(0, 4, false);
            data.setUint8(1, override, false);
            data.setUint16(2, targetIK.x, false);
            data.setUint16(4, targetIK.y, false);
            data.setUint16(6, targetIK.z, false);
        } else {
            targetAngles.x = clamp(targetAngles.x + armLeft.x * JOINT_SPEED * deltaT, 0, 1);
            targetAngles.j1 = clamp(targetAngles.j1 + armLeft.y * JOINT_SPEED * deltaT, 0, 1);
            targetAngles.j2 = clamp(targetAngles.j2 + armRight.x * JOINT_SPEED * deltaT, 0, 1);
            targetAngles.j3 = clamp(targetAngles.j3 + armRight.y * JOINT_SPEED * deltaT, 0, 1);

            data.setUint8(0, 3, false);
            data.setUint8(1, override, false);
            data.setUint16(2, targetAngles.x * 0x10000, false);
            data.setUint16(4, targetAngles.j1 * 0x10000, false);
            data.setUint16(6, targetAngles.j2 * 0x10000, false);
            data.setUint16(8, targetAngles.j3 * 0x10000, false);
        }
        if (socket.readyState === WebSocket.OPEN) socket.send(buffer);
    }, COMMAND_INTERVAL * 1000);
};