const JOYSTICK_CLIP_RANGE = 0.2;
const GAMMA = 1.5;
const JOINT_SPEED = 0.02; // range/s
const IK_SPEED = 4; // mm/s
const ACTIVE_TIMEOUT = 1; // s to send drive and/or arm commands after their joysticks are released, must be at least 2 * COMMAND_INTERVAL
const COMMAND_INTERVAL = 0.1; // s
const LCD_WIDTH = 320;
const LCD_HEIGHT = 240;

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

    let authorized = false;
    let targetAngles = {
        x: 0,
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
    let driveActiveUntil = 0;
    let armActiveUntil = 0;
    let override = false;
    let locked = true;

    const drive = new Joystick("drive");
    const armLeft = new Joystick("arm-left");
    const armRight = new Joystick("arm-right");
    const lock = document.getElementById("lock");
    const on = document.getElementById("on");
    const off = document.getElementById("off");
    const overrideElements = Array.from(document.getElementsByClassName("override"));

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
            const data = new DataView(buffer);
            data.setUint8(0, 1, true);
            data.setUint8(1, override, true);
            socket.send(buffer);
        }
    });
    off.addEventListener("click", _ => {
        if (socket.readyState === WebSocket.OPEN) {
            const buffer = new ArrayBuffer(1);
            const data = new DataView(buffer);
            data.setUint8(0, 0, true);
            data.setUint8(1, override, true);
            socket.send(buffer);
        }
    });

    let uploadedImageNames = [];
    let uploadedImages = [];
    const display = document.getElementById("display");
    IMAGE_NAMES.forEach(name => display.appendChild(new Option(name)));
    display.addEventListener("change", _ => {
        if (display.selectedIndex < IMAGE_NAMES.length) {
            if (socket.readyState === WebSocket.OPEN) {
                const buffer = new ArrayBuffer(2);
                const data = new DataView(buffer);
                data.setUint8(0, 5, true);
                data.setUint8(1, override, true);
                data.setUint8(2, display.selectedIndex, true);
                socket.send(buffer);
            }
        } else {
            fetch("/display", { method: "POST", body: uploadedImages[display.selectedIndex - IMAGE_NAMES.length] });
        }
    });

    const upload = document.getElementById("upload");
    upload.addEventListener("change", _ => {
        const fileReader = new FileReader();
        const idx = uploadedImageNames.length;
        fileReader.onload = () => {
            const image = new Image();
            image.src = fileReader.result;

            image.onload = () => {
                // Scale preserves aspect ratio.
                const scale = Math.min(
                    LCD_WIDTH / image.naturalWidth,
                    LCD_HEIGHT / image.naturalHeight
                );

                const scaledWidth = image.naturalWidth * scale;
                const scaledHeight = image.naturalHeight * scale;

                // Center image.
                const x = (LCD_WIDTH - scaledWidth) / 2;
                const y = (LCD_HEIGHT - scaledHeight) / 2;

                // Preform anti-aliasing by bluring image by 1 / (scale * 2)
                const antiAliasingCanvas = document.createElement("canvas");
                const antiAliasingCanvasContext = antiAliasingCanvas.getContext("2d");
                antiAliasingCanvas.width = image.width;
                antiAliasingCanvas.height = image.height;
                antiAliasingCanvasContext.filter = `blur(${(1 / scale) >> 1}px)`;
                antiAliasingCanvasContext.drawImage(image, 0, 0);


                // Assemble background and scale and translate image.
                const canvas = document.createElement("canvas");
                const canvasContext = canvas.getContext("2d");
                canvas.width = LCD_WIDTH;
                canvas.height = LCD_HEIGHT;

                canvasContext.fillStyle = "#000";
                canvasContext.fillRect(0, 0, LCD_WIDTH, LCD_HEIGHT);
                canvasContext.drawImage(antiAliasingCanvas, 0, 0, image.width, image.height, x, y, scaledWidth, scaledHeight);

                // Convert image data.
                const rawImageData = canvasContext.getImageData(0, 0, LCD_WIDTH, LCD_HEIGHT, { "colorSpace": "srgb", "pixelFormat": "rgba-unorm8" }).data;
                const convertedImageData = new Uint8Array(LCD_WIDTH * LCD_HEIGHT * 2);
                for (let i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
                    let r = rawImageData[i * 4];
                    let g = rawImageData[i * 4 + 1];
                    let b = rawImageData[i * 4 + 2];
                    // r4 r3 r2 r1 r0 g5 g4 g3 = r7 r6 r5 r4 r3 g7 g6 g5
                    convertedImageData[i * 2] = (r & 0b11111000) | ((g >> 5) & 0b00000111);
                    // g2 g1 g0 b4 b3 b2 b1 b0 = g4 g3 g2 b7 b6 b5 b4 b3
                    convertedImageData[i * 2 + 1] = ((g << 3) & 0b11100000) | ((b >> 3) & 0b00011111);
                }

                uploadedImages[idx] = convertedImageData;
            }
        }
        uploadedImageNames[idx] = upload.files[0].name;
        display.appendChild(new Option(upload.files[0].name));
        fileReader.readAsDataURL(upload.files[0]);
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
        "aj2": [0, 0, 100, 100, ["@% J2", 3, 0]],
        "aj3": [0, 0, 100, 100, ["@% J3", 3, 0]],
        "aex": [-500, -500, 500, 500, ["@mm X", 3, 0]],
        "aey": [-500, -500, 500, 500, ["@mm Y", 3, 0]],
        "aez": [-500, -500, 500, 500, ["@mm Z", 3, 0]],
    };
    Object.keys(telemetry).forEach(id => {
        telemetry[id] = new Bar(id, ...telemetry[id]);
    });

    socket.addEventListener("message", event => {
        let data = new DataView(event.data);
        telemetry.bv.update(data.getFloat32(16, true));
        telemetry.ba.update(data.getFloat32(20, true));
        telemetry.c1.update(data.getFloat32(24, true));
        telemetry.c2.update(data.getFloat32(28, true));
        telemetry.c3.update(data.getFloat32(32, true));
        telemetry.c4.update(data.getFloat32(36, true));
        telemetry.dl.update(Math.abs(data.getInt16(40, true) / 0x7000 * 100));
        telemetry.dr.update(Math.abs(data.getInt16(42, true) / 0x7000 * 100));
        let ax = data.getUint16(44, true) / 0x10000;
        let aj2 = data.getUint16(46, true) / 0x10000;
        let aj3 = data.getUint16(48, true) / 0x10000;
        let aex = data.getInt16(50, true);
        let aey = data.getInt16(52, true);
        let aez = data.getInt16(54, true);

        telemetry.ax.update(ax * 100);
        telemetry.aj2.update(aj2 * 100);
        telemetry.aj3.update(aj3 * 100);
        telemetry.aex.update(aex);
        telemetry.aey.update(aey);
        telemetry.aez.update(aez);

        // Set the non-controlled target values from telemetry.
        if (ik) {
            targetAngles.x = ax;
            targetAngles.j2 = j2;
            targetAngles.j3 = j3;
        } else {
            targetIK.x = aex;
            targetIK.y = aey;
            targetIK.z = aez;
        }
    });

    setInterval(() => {
        let now = performance.now();
        let deltaT = (now - lastTime) * 1000;
        lastTime = now;

        let driveSpeed = 1; // TODO: change max drive speed.
        let driveX = drive.x;
        let driveY = drive.y;
        // Scale exponentially.
        driveX = driveX < 0 ? -Math.pow(-driveX, GAMMA) : Math.pow(driveX, GAMMA);
        driveY = driveY < 0 ? -Math.pow(-driveY, GAMMA) : Math.pow(driveY, GAMMA);
        let [leftSpeed, rightSpeed] = arcadeDrive(driveX, driveY);
        leftSpeed = clamp(leftSpeed, -1, 1, -driveSpeed, driveSpeed);
        rightSpeed = clamp(rightSpeed, -1, 1, -driveSpeed, driveSpeed);

        let buffer = new ArrayBuffer(8);
        let data = new DataView(buffer);

        if (drive.active) driveActiveUntil = now + ACTIVE_TIMEOUT * 1000;
        if (socket.readyState === WebSocket.OPEN && driveActiveUntil > now) {
            data.setUint8(0, 2, true);
            data.setUint8(1, override, true);
            data.setInt16(2, leftSpeed * 0x7000, true);
            data.setInt16(4, rightSpeed * 0x7000, true);
            socket.send(buffer);
        }

        if (armLeft.active || armRight.active) armActiveUntil = now + ACTIVE_TIMEOUT * 1000;
        if (socket.readyState === WebSocket.OPEN && armActiveUntil > now) {
            if (ik) {
                targetAngles.x = clamp(targetAngles.x + armRight.x * IK_SPEED * deltaT, 0, 500);
                targetAngles.j2 = clamp(targetAngles.j2 + armLeft.x * IK_SPEED * deltaT, 0, 500);
                targetAngles.j3 = clamp(targetAngles.j3 + armLeft.y * IK_SPEED * deltaT, 0, 500);

                data.setUint8(0, 4, true);
                data.setUint8(1, override, true);
                data.setInt16(2, targetIK.x, true);
                data.setInt16(4, targetIK.y, true);
                data.setInt16(6, targetIK.z, true);
            } else {
                targetAngles.x = clamp(targetAngles.x + armLeft.x * JOINT_SPEED * deltaT, 0, 1);
                targetAngles.j2 = clamp(targetAngles.j2 + armRight.x * JOINT_SPEED * deltaT, 0, 1);
                targetAngles.j3 = clamp(targetAngles.j3 + armRight.y * JOINT_SPEED * deltaT, 0, 1);

                data.setUint8(0, 3, true);
                data.setUint8(1, override, true);
                data.setUint16(2, targetAngles.x * 0x10000, true);
                data.setUint16(4, targetAngles.j2 * 0x10000, true);
                data.setUint16(6, targetAngles.j3 * 0x10000, true);
            }
            socket.send(buffer);
        }
    }, COMMAND_INTERVAL * 1000);
};
