"use strict";
window.addEventListener("load", () => {
    const image = document.getElementById("map-image");
    const canvas = document.getElementById("map-canvas");
    const ctx = canvas.getContext("2d");
    if (!image || !canvas || !ctx)
        return;
    canvas.width = image.clientWidth;
    canvas.height = image.clientHeight;
    const cellSize = 50;
    ctx.strokeStyle = "#cccccc";
    ctx.lineWidth = 1;
    ctx.font = "10px Arial";
    ctx.fillStyle = "#444";
    for (let x = 0; x <= canvas.width; x += cellSize) {
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, canvas.height);
        ctx.stroke();
        if (x > 0)
            ctx.fillText(`${x}`, x + 2, 10);
    }
    for (let y = 0; y <= canvas.height; y += cellSize) {
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(canvas.width, y);
        ctx.stroke();
        if (y > 0)
            ctx.fillText(`${y}`, 2, y - 2);
    }
    const dataElement = document.getElementById("points-data");
    if (dataElement) {
        const points = JSON.parse(dataElement.textContent || "[]");
        ctx.fillStyle = "red";
        points.forEach((p) => {
            ctx.beginPath();
            ctx.arc(p.x, p.y, 4, 0, 2 * Math.PI);
            ctx.fill();
        });
    }
    canvas.addEventListener("click", (event) => {
        const rect = canvas.getBoundingClientRect();
        const x = event.clientX - rect.left;
        const y = event.clientY - rect.top;
        ctx.fillStyle = "blue";
        ctx.beginPath();
        ctx.arc(x, y, 4, 0, 2 * Math.PI);
        ctx.fill();
        const inputX = document.querySelector("input[name='X-cord']");
        const inputY = document.querySelector("input[name='Y-cord']");
        if (inputX && inputY) {
            inputX.value = String(Math.round(x));
            inputY.value = String(Math.round(y));
        }
    });
    const imageContainer = document.getElementById("map-container");
    let highlightDot = null;
    function showIndicator(x, y) {
        removeIndicator();
        highlightDot = document.createElement("div");
        highlightDot.style.position = "absolute";
        highlightDot.style.left = `${x - 10}px`;
        highlightDot.style.top = `${y - 10}px`;
        highlightDot.style.width = "20px";
        highlightDot.style.height = "20px";
        highlightDot.style.borderRadius = "50%";
        highlightDot.style.backgroundColor = "green";
        highlightDot.style.pointerEvents = "none";
        highlightDot.style.zIndex = "999";
        imageContainer.appendChild(highlightDot);
    }
    function removeIndicator() {
        if (highlightDot) {
            highlightDot.remove();
            highlightDot = null;
        }
    }
    document.querySelectorAll(".point-item").forEach((el) => {
        el.addEventListener("mouseover", () => {
            const x = parseFloat(el.getAttribute("data-x") || "0");
            const y = parseFloat(el.getAttribute("data-y") || "0");
            showIndicator(x, y);
        });
        el.addEventListener("mouseout", () => {
            removeIndicator();
        });
    });
});
//# sourceMappingURL=drawPoint.js.map