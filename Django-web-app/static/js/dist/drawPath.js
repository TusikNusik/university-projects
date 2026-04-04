var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var _a, _b;
let path = [...initialPath];
const grid = document.getElementById("grid");
const rows = Number(grid.getAttribute("data-rows")) || 10;
const cols = Number(grid.getAttribute("data-cols")) || 10;
function renderGrid() {
    grid.innerHTML = "";
    grid.style.display = "grid";
    grid.style.gridTemplateRows = `repeat(${rows}, 40px)`;
    grid.style.gridTemplateColumns = `repeat(${cols}, 40px)`;
    for (let r = 0; r < rows; r++) {
        for (let c = 0; c < cols; c++) {
            const cell = document.createElement("div");
            cell.dataset.row = r.toString();
            cell.dataset.col = c.toString();
            cell.style.border = "1px solid #ccc";
            cell.style.width = "40px";
            cell.style.height = "40px";
            cell.style.display = "flex";
            cell.style.alignItems = "center";
            cell.style.justifyContent = "center";
            const dot = initialDots.find(d => d.row === r && d.col === c);
            if (dot)
                cell.style.backgroundColor = dot.color;
            const index = path.findIndex(p => p.row === r && p.col === c);
            if (index !== -1) {
                cell.textContent = (index + 1).toString();
                cell.style.backgroundColor = "#000";
                cell.style.color = "white";
            }
            cell.addEventListener("click", () => {
                if (path.some(p => p.row === r && p.col === c))
                    return;
                if (path.length === 0) {
                    path.push({ row: r, col: c });
                    renderGrid();
                    return;
                }
                const last = path[path.length - 1];
                const dr = Math.abs(last.row - r);
                const dc = Math.abs(last.col - c);
                if ((dr === 1 && dc === 0) || (dr === 0 && dc === 1)) {
                    path.push({ row: r, col: c });
                    renderGrid();
                }
                else {
                    alert("You can select only a neighboring cell.");
                }
            });
            grid.appendChild(cell);
        }
    }
}
renderGrid();
(_a = document.getElementById("undoPath")) === null || _a === void 0 ? void 0 : _a.addEventListener("click", () => {
    if (path.length > 0) {
        path.pop();
        renderGrid();
    }
});
(_b = document.getElementById("clearPath")) === null || _b === void 0 ? void 0 : _b.addEventListener("click", () => {
    if (confirm("Remove the entire path?")) {
        path = [];
        renderGrid();
    }
});
document.getElementById("savePath").addEventListener("click", () => __awaiter(void 0, void 0, void 0, function* () {
    const response = yield fetch(`/route/drawPath/${boardId}`, {
        method: "POST",
        headers: {
            "Content-Type": "application/json",
            "X-CSRFToken": getCSRFToken()
        },
        body: JSON.stringify({ path })
    });
    if (response.ok) {
        alert("Path saved.");
    }
    else {
        alert("Unable to save the path.");
    }
}));
function getCSRFToken() {
    const name = "csrftoken";
    const cookie = document.cookie.split("; ").find(row => row.startsWith(name + "="));
    return cookie ? decodeURIComponent(cookie.split("=")[1]) : "";
}
export {};
//# sourceMappingURL=drawPath.js.map