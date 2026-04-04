"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
let selectedColor = "#ff0000";
let placedDots = [...initialDots];
const gridDiv = document.getElementById("grid");
const colorPicker = document.getElementById("colorPicker");
const rowsInput = document.getElementById("rows");
const colsInput = document.getElementById("cols");
const generateButton = document.getElementById("generate-grid");
const saveButton = document.getElementById("saveBoard");
colorPicker.addEventListener("input", () => {
    selectedColor = colorPicker.value;
});
generateButton.addEventListener("click", () => {
    renderGrid();
});
saveButton.addEventListener("click", () => __awaiter(void 0, void 0, void 0, function* () {
    const payload = {
        rows: parseInt(rowsInput.value),
        cols: parseInt(colsInput.value),
        dots: placedDots
    };
    yield fetch(`/route/editBoard/${boardId}`, {
        method: "POST",
        headers: {
            "Content-Type": "application/json",
            "X-CSRFToken": getCSRFToken()
        },
        body: JSON.stringify(payload)
    });
}));
function renderGrid() {
    const rows = parseInt(rowsInput.value);
    const cols = parseInt(colsInput.value);
    gridDiv.innerHTML = "";
    gridDiv.style.display = "grid";
    gridDiv.style.gridTemplateRows = `repeat(${rows}, 40px)`;
    gridDiv.style.gridTemplateColumns = `repeat(${cols}, 40px)`;
    for (let r = 0; r < rows; r++) {
        for (let c = 0; c < cols; c++) {
            const cell = document.createElement("div");
            cell.dataset.row = r.toString();
            cell.dataset.col = c.toString();
            cell.style.border = "1px solid #ccc";
            cell.style.width = "40px";
            cell.style.height = "40px";
            const existing = placedDots.find(d => d.row === r && d.col === c);
            if (existing) {
                cell.style.backgroundColor = existing.color;
                cell.classList.add("dot");
            }
            cell.addEventListener("click", () => handleClickCell(r, c, cell));
            gridDiv.appendChild(cell);
        }
    }
}
function handleClickCell(row, col, cell) {
    const colorDots = placedDots.filter(d => d.color === selectedColor);
    if (cell.classList.contains("dot"))
        return;
    if (colorDots.length >= 2)
        return;
    placedDots.push({ row, col, color: selectedColor });
    cell.style.backgroundColor = selectedColor;
    cell.classList.add("dot");
}
function getCSRFToken() {
    const name = "csrftoken";
    const cookie = document.cookie
        .split("; ")
        .find(row => row.startsWith(name + "="));
    return cookie ? decodeURIComponent(cookie.split("=")[1]) : "";
}
renderGrid();
//# sourceMappingURL=editBoard.js.map