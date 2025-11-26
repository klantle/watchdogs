#include <a_samp>

main() {}

public OnPlayerCommandText(playerid, cmdtext[])
{
	if (strcmp(cmdtext, "/help") == 0)
		{
			SendClientMessage(
				playerid,
				-1,
				"help:" \
				"/command" \
				"/command2" \
				"/command3"
				);
			return 1; /* success */
		}
	return 0;
}