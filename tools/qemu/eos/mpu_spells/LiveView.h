/* PROP_SHOOTING_TYPE 3 is LiveView */
/* this reply appears to be enough to show GUI elements in LiveView
 * however, the actual handshake is a lot more complex,
 * but it appears to (re)initialize some properties
 */

    { { 0x08, 0x06, 0x04, 0x0c, ARG0, ARG1, 0x00 }, .description = "PROP_SHOOTING_TYPE", .out_spells = { /* spell #50 */
        { 0x08, 0x06, 0x04, 0x0c, ARG0, ARG1, 0x01 },           /* reply #53.1, PROP_SHOOTING_TYPE */
        { 0 } } },
