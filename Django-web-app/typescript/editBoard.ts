interface Dot {
    row: number;
    col: number;
    color: string;
}

declare const boardId: number;
declare const initialDots: Dot[];

let selectedColor = "#ff0000";
let placedDots: Dot[] = [...initialDots];

const gridDiv = document.getElementById("grid")!;
const colorPicker = document.getElementById("colorPicker") as HTMLInputElement;
const rowsInput = document.getElementById("rows") as HTMLInputElement;
const colsInput = document.getElementById("cols") as HTMLInputElement;
const generateButton = document.getElementById("generate-grid")!;
const saveButton = document.getElementById("saveBoard")!;

colorPicker.addEventListener("input", () => {
    selectedColor = colorPicker.value;
});

generateButton.addEventListener("click", () => {
    renderGrid();
});

saveButton.addEventListener("click", async () => {
    const payload = {
        rows: parseInt(rowsInput.value),
        cols: parseInt(colsInput.value),
        dots: placedDots
    };

    await fetch(`/route/editBoard/${boardId}`, {
        method: "POST",
        headers: {
            "Content-Type": "application/json",
            "X-CSRFToken": getCSRFToken()
        },
        body: JSON.stringify(payload)
    });
});

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

function handleClickCell(row: number, col: number, cell: HTMLElement) {
    const colorDots = placedDots.filter(d => d.color === selectedColor);

    if (cell.classList.contains("dot")) return;
    if (colorDots.length >= 2) return;

    placedDots.push({ row, col, color: selectedColor });
    cell.style.backgroundColor = selectedColor;
    cell.classList.add("dot");
}

function getCSRFToken(): string {
    const name = "csrftoken";
    const cookie = document.cookie
        .split("; ")
        .find(row => row.startsWith(name + "="));
    return cookie ? decodeURIComponent(cookie.split("=")[1]) : "";
}

renderGrid();
