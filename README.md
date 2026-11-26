# LIN-DEMO

## Project overview

This embedded C project demonstrates LIN communication on an NXP S32K312
microcontroller. It includes a LIN driver, GPIO and PIT timer support, and a
UART6 console used by the master configuration to report LIN transmissions and
responses. The sample supports operating as a LIN master, a LIN slave, or both
roles in one firmware image. The build outputs and generated configuration are
under `hw04_LIN/`.

## LIN node role configurations

Select the firmware role by setting `LIN_NODE_ROLE` in
`hw04_LIN/src/main.c`. The available role constants are:

| Setting | Behavior |
| --- | --- |
| `LIN_NODE_MASTER` | Initializes the LIN master, SW3 button handling, and UART6. On each SW3 press, the master sends an 8-byte LIN frame to the selected ID. The transaction callback saves the response, and the main loop prints the result over UART6. |
| `LIN_NODE_SLAVE` | Initializes the LIN slave, LED, and timer. On a request for `ACTIVE_SLAVE_ID`, the slave turns on its configured LED and prepares its response payload. Requests for other IDs are assigned a response length of zero. The timer callback turns the configured LED off. |
| `LIN_NODE_BOTH` | Initializes both master and slave peripherals and executes both roles. It handles master button presses and response logging while also servicing slave requests and the LED timer. Use this when the board is intended to perform both roles in the same application. |

The role constants are numeric (`LIN_NODE_MASTER` = `0`, `LIN_NODE_SLAVE` =
`1`, `LIN_NODE_BOTH` = `2`). The source currently selects `LIN_NODE_BOTH` by
default. Change that definition to the desired role before building.

### Related settings

- `ACTIVE_SLAVE_ID` selects the slave ID that responds and the LED/payload variant used by the slave callback.
- `SLAVE_ID_1`, `SLAVE_ID_2`, and `SLAVE_ID_3` define the IDs in the master's request list.
- `SLAVE_RESPONSE_LENGTH` sets the requested response length; the master sends frames with this length.

The master request-ID counter is intended to rotate through the IDs in
`slave_ids`. The current wrap condition uses `>` rather than `>=` when compared
with the list length, so repeated requests can advance the index out of bounds.
Correct that condition before relying on ID rotation.