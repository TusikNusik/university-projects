# The Great Approximator (Wielki Aproksymator)

## Project Overview
"The Great Approximator" is a TCP-based client-server game. Players (represented by client programs) compete to best approximate a polynomial provided by the server. The polynomial is defined as:

$$f(x) = \sum_{i=0}^N a_i x^i$$

The approximation is evaluated at integer points from $0$ to $K$. Initially, the approximating function $\hat{f}(x)$ has a value of $0$ at all points. Players iteratively send values to be added to specific points. The player with the lowest penalty score at the end of the game wins.

The score (penalty) is calculated as the sum of squared deviations from the expected values:

$$\sum_{x=0}^K (\hat{f}(x) - f(x))^2$$

Additional penalties are added for invalid moves or protocol violations. The game ends after the server processes a total of $M$ valid additions.

---

## Building and Running

### Server
The server handles multiple clients concurrently via TCP (IPv4 and IPv6) without blocking.

**Command:**
`approx-server [options] -f file`

**Parameters:**
* `-p port`: Server port (0-65535, default: 0 for random port).
* `-k value`: Constant $K$ (1-10000, default: 100).
* `-n value`: Constant $N$ (1-8, default: 4).
* `-m value`: Constant $M$, total valid moves to end the game (1-12341234, default: 131).
* `-f file`: **(Required)** Path to the file containing polynomial coefficients.

### Client
The client connects to the server and interacts either manually via standard input or automatically.

**Command:**
`approx-client -u player_id -s server -p port [options]`

**Parameters:**
* `-u player_id`: **(Required)** Alphanumeric player identifier.
* `-s server`: **(Required)** Server address or hostname.
* `-p port`: **(Required)** Server port (1-65535).
* `-4`: Force IPv4 communication.
* `-6`: Force IPv6 communication.
* `-a`: Enable automatic strategy mode (must be better than random). If omitted, the client reads `$point $value` pairs from standard input.

---

## Communication Protocol

All communication is text-based. Every message must end with `\r\n` (CRLF). Fields are separated by a single space. Decimal numbers can have up to 7 decimal places.

### Connection & Initialization
1.  **Client connects** and must send within 3 seconds:
    `HELLO $player_id\r\n`
2.  **Server responds** with the polynomial coefficients (read from the `-f` file):
    `COEFF $a_0 $a_1 ... $a_N\r\n`
    *(Coefficients are rational numbers between -100 and 100).*

### Gameplay
1.  **Client sends a move** to add `$value` at `$point`:
    `PUT $point $value\r\n`
    * `$point`: Integer between $0$ and $K$.
    * `$value`: Rational number between -5 and 5.

2.  **Server processes the move**:
    * **Valid move:** Server updates $\hat{f}(x)$, delays the response by $X$ seconds (where $X$ is the number of lowercase letters in `$player_id`), and responds with the current state:
        `STATE $r_0 ... $r_K\r\n`
    * **Invalid values ($point out of bounds, etc.):** Server waits 1 second, applies a 10-point penalty, and responds:
        `BAD_PUT $point $value\r\n`
    * **Protocol Violation (Sending PUT before COEFF or before the previous STATE response):** Server applies a 20-point penalty and responds immediately:
        `PENALTY $point $value\r\n`

### Game End
Once the server has processed $M$ valid `PUT` messages across all connected clients, it broadcasts the final scores:

`SCORING $player_id_1 $result_1 $player_id_2 $result_2 ...\r\n`

* Player IDs are sorted lexicographically.
* After sending this, the server disconnects all clients, resets its state, waits 1 second, and starts over (without resetting the position in the coefficients file).
* Upon receiving `SCORING`, clients exit with code 0.

---

## Error Handling & Diagnostics

* **Standard Errors:** Invalid parameters, bad input lines, or unexpected disconnections print an error to `stderr` formatted as `ERROR: $error_description\n` and exit with code 1.
* **Protocol Errors:** Unexpected or malformed messages trigger an error log:
    `ERROR: bad message from [$ip_address]:$port, $player_id: $message\n`
    *(If `$player_id` is unknown, `UNKNOWN` is used).*