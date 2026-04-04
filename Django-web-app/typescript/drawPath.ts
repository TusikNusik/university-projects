export
interface Dot {
    row: number;
    col: number;
    color: string;
}

interface PathPoint {
    row: number;
    col: number;
}

declare const boardId: number;
declare const initialDots: Dot[];
declare const initialPath: PathPoint[];

let path: PathPoint[] = [...initialPath];
const grid = document.getElementById("grid")!;
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
            if (dot) cell.style.backgroundColor = dot.color;

            const index = path.findIndex(p => p.row === r && p.col === c);
            if (index !== -1) {
                cell.textContent = (index + 1).toString();
                cell.style.backgroundColor = "#000";
                cell.style.color = "white";
            }

            cell.addEventListener("click", () => {
                if (path.some(p => p.row === r && p.col === c)) return;

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
                } else {
                    alert("You can select only a neighboring cell.");
                }
            });

            grid.appendChild(cell);
        }
    }
}

renderGrid();

document.getElementById("undoPath")?.addEventListener("click", () => {
    if (path.length > 0) {
        path.pop();
        renderGrid();
    }
});

document.getElementById("clearPath")?.addEventListener("click", () => {
    if (confirm("Remove the entire path?")) {
        path = [];
        renderGrid();
    }
});

document.getElementById("savePath")!.addEventListener("click", async () => {
    const response = await fetch(`/route/drawPath/${boardId}`, {
        method: "POST",
        headers: {
            "Content-Type": "application/json",
            "X-CSRFToken": getCSRFToken()
        },
        body: JSON.stringify({ path })
    });

    if (response.ok) {
        alert("Path saved.");
    } else {
        alert("Unable to save the path.");
    }
});

function getCSRFToken(): string {
    const name = "csrftoken";
    const cookie = document.cookie.split("; ").find(row => row.startsWith(name + "="));
    return cookie ? decodeURIComponent(cookie.split("=")[1]) : "";
}
