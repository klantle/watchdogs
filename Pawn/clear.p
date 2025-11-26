#include <a_samp>

main() {
	new i; /* init */
	new val_clear = 99; /* 99 hidden message */
	new param_player = 0; /* playerid parameter - testing = 0 */
	new color_message = -1; /* message color - testing -1 (reset/white) */
	for (i = 0 /* reset */; i < val_clear; ++i) {
		SendClientMessage(param_player, color_message, ""); // empty message
	}
}