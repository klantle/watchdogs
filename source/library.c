#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stddef.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "extra.h"
#include "utils.h"

#if __has_include(<glib.h>)
    #include <glib.h>
#elif __has_include(<glib-2.0/glib.h>)
    #include <glib-2.0/glib.h>
#endif

#if __has_include(<gtk/gtk.h>)
    #include <gtk/gtk.h>
#elif __has_include(<gtk-3.0/gtk/gtk.h>)
    #include <gtk-3.0/gtk/gtk.h>
#elif __has_include(<gtk.h>)
    #include <gtk.h>
#endif

#include "units.h"
#include "archive.h"
#include "curl.h"
#include "debug.h"
#include "library.h"

static char gtk_selected_key = 0;

static void on_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer data)
{
      GtkWidget *window = GTK_WIDGET(data);
      gtk_selected_key = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "key"));
      gtk_widget_destroy(window);
}

static char
gtk_select_fullscreen(const char *title, const char **items, const char *keys, int counts)
{
      static int inited = 0;
      if (!inited) {
          int argc = 0;
          char **argv = NULL;
          gtk_init(&argc, &argv);
          inited = 1;
      }

      GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_window_set_title(GTK_WINDOW(window), title);
      gtk_window_fullscreen(GTK_WINDOW(window));
      gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

      g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

      GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
      gtk_container_add(GTK_CONTAINER(window), scroll);

      GtkWidget *list = gtk_list_box_new();
      gtk_container_add(GTK_CONTAINER(scroll), list);

      for (int i = 0; i < counts; i++) {
          GtkWidget *row = gtk_list_box_row_new();
          GtkWidget *label = gtk_label_new(items[i]);
          gtk_label_set_xalign(GTK_LABEL(label), 0.0);
          gtk_container_add(GTK_CONTAINER(row), label);

          g_object_set_data(G_OBJECT(row), "key", GINT_TO_POINTER(keys[i]));
          gtk_list_box_insert(GTK_LIST_BOX(list), row, -1);
      }

      g_signal_connect(list, "row-activated", G_CALLBACK(on_row_activated), window);

      gtk_widget_show_all(window);
      gtk_main();

      return gtk_selected_key;
}

static char cli_select(const char *title, const char **items, const char *keys, int counts)
{
      printf("\n%s\n", title);
      for (int i = 0; i < counts; i++) {
            printf("  %c) %s\n", keys[i], items[i]);
      }

      while (true) {
            char *input = NULL;
            input = readline("Select option: ");
            if (!input)
                continue;

            char choice = '\0';
            if (strlen(input) == 1) {
                  choice = input[0];
                  int k;
                  for (k = 0; k < counts; k++) if (choice == keys[k] || choice == (keys[k] + 32))
                  		{
                              wg_free(input);
                              return choice;
                        }
            }

            printf("Invalid selection. Please try again.\n");
            wg_free(input);
      }
}

static char select_interface(const char *title, const char **items, const char *keys, int counts)
{
      printf("\nSelect Interface:\n");
      printf("  1) GUI (Graphical User Interface)\n");
      printf("  2) CLI (Command Line Interface)\n");

      while (true) {
            char *input = NULL;
			input = readline("Choose interface (1 or 2): ");
			if (!input)
					continue;

            if (strlen(input) == 1) {
                  if (input[0] == '1') {
                        wg_free(input);
                        return gtk_select_fullscreen(title, items, keys, counts);
                  } else if (input[0] == '2') {
                        wg_free(input);
                        return cli_select(title, items, keys, counts);
                  }
            }

            printf("Invalid choice. Please enter 1 or 2.\n");
            wg_free(input);
      }
}

static int pawncc_handle_termux_installation(void)
{
      const char *items[] = {
          "Pawncc 3.10.11",
          "Pawncc 3.10.10",
          "Pawncc 3.10.9",
          "Pawncc 3.10.8",
          "Pawncc 3.10.7"
      };
      const char keys[] = { 'A','B','C','D','E' };

      char sel = select_interface("Select PawnCC Version", items, keys, 5);
      if (!sel) return 0;

      wgconfig.wg_ipawncc = 1;

      if (sel == 'A' || sel == 'a') {
          if (path_exists("pawncc-termux-311.zip")) remove("pawncc-termux-311.zip");
          if (path_exists("pawncc-termux-311")) remove("pawncc-termux-311");
          wg_download_file("https://github.com/gskeleton/compiler/releases/download/3.10.11/pawncc-termux.zip","pawncc-termux-311.zip");
      } else if (sel == 'B' || sel == 'b') {
          if (path_exists("pawncc-termux-310.zip")) remove("pawncc-termux-310.zip");
          if (path_exists("pawncc-termux-310")) remove("pawncc-termux-310");
          wg_download_file("https://github.com/gskeleton/compiler/releases/download/3.10.10/pawncc-termux.zip","pawncc-termux-310.zip");
      } else if (sel == 'C' || sel == 'c') {
          if (path_exists("pawncc-termux-39.zip")) remove("pawncc-termux-39.zip");
          if (path_exists("pawncc-termux-39")) remove("pawncc-termux-39");
          wg_download_file("https://github.com/gskeleton/compiler/releases/download/3.10.9/pawncc-termux.zip","pawncc-termux-39.zip");
      } else if (sel == 'D' || sel == 'd') {
          if (path_exists("pawncc-termux-38.zip")) remove("pawncc-termux-38.zip");
          if (path_exists("pawncc-termux-38")) remove("pawncc-termux-38");
          wg_download_file("https://github.com/gskeleton/compiler/releases/download/3.10.8/pawncc-termux.zip","pawncc-termux-38.zip");
      } else if (sel == 'E' || sel == 'e') {
          if (path_exists("pawncc-termux-37.zip")) remove("pawncc-termux-37.zip");
          if (path_exists("pawncc-termux-37")) remove("pawncc-termux-37");
          wg_download_file("https://github.com/gskeleton/compiler/releases/download/3.10.7/pawncc-termux.zip","pawncc-termux-37.zip");
      }

      return 0;
}

static int pawncc_handle_standard_installation(const char *platform)
{
      const char *versions[] = {
          "PawnCC 3.10.11", "PawnCC 3.10.10", "PawnCC 3.10.9", "PawnCC 3.10.8",
          "PawnCC 3.10.7", "PawnCC 3.10.6", "PawnCC 3.10.5", "PawnCC 3.10.4",
          "PawnCC 3.10.3", "PawnCC 3.10.2"
      };
      const char keys[] = { 'A','B','C','D','E','F','G','H','I','J' };
      const char *vernums[] = {
          "3.10.11","3.10.10","3.10.9","3.10.8","3.10.7",
          "3.10.6","3.10.5","3.10.4","3.10.3","3.10.2"
      };

      if (strcmp(platform, "linux") != 0 && strcmp(platform, "windows") != 0) {
          pr_error(stdout, "Unsupported platform: %s", platform);
          return -1;
      }

      char sel = select_interface("Select PawnCC Version", versions, keys, 10);
      if (!sel) return 0;

      int idx = -1;
      if (sel >= 'A' && sel <= 'J') idx = sel - 'A';
      else if (sel >= 'a' && sel <= 'j') idx = sel - 'a';
      if (idx < 0 || idx >= 10) return 0;

      const char *library_repo_base;
      if (strcmp(vernums[idx], "3.10.11") == 0)
          library_repo_base = "https://github.com/openmultiplayer/compiler";
      else
          library_repo_base = "https://github.com/pawn-lang/compiler";

      const char *archive_ext = (strcmp(platform, "linux") == 0) ? "tar.gz" : "zip";

      char url[512], filename[128];
      snprintf(url, sizeof(url),
          "%s/releases/download/v%s/pawnc-%s-%s.%s",
          library_repo_base, vernums[idx], vernums[idx], platform, archive_ext);

      snprintf(filename, sizeof(filename),
          "pawnc-%s-%s.%s", vernums[idx], platform, archive_ext);

      wgconfig.wg_ipawncc = 1;
      wg_download_file(url, filename);

      return 0;
}

int wg_install_pawncc(const char *platform)
{
  		__debug_function();

  		if (!platform) {
  				pr_error(stdout, "Platform parameter is NULL");
  				if (wgconfig.wg_sel_stat == 0)
  					return 0;
  				return -1;
  		}

  		if (strcmp(platform, "termux") == 0) {
  			int ret = pawncc_handle_termux_installation();

loop_ipcc:
  			if (ret == -1 && wgconfig.wg_sel_stat != 0)
  				goto loop_ipcc;
  			else if (ret == 0)
  				return 0;
  		} else {
  			int ret = pawncc_handle_standard_installation(platform);
loop_ipcc2:
  			if (ret == -1 && wgconfig.wg_sel_stat != 0)
  				goto loop_ipcc2;
  			else if (ret == 0)
  				return 0;
  		}

  		return 0;
}

int wg_install_server(const char *platform)
{
      __debug_function();

      if (strcmp(platform, "linux") != 0 && strcmp(platform, "windows") != 0) {
          pr_error(stdout, "Unsupported platform: %s", platform);
          return -1;
      }

      const char *items[] = {
          "SA-MP 0.3.DL R1",
          "SA-MP 0.3.7 R3",
          "SA-MP 0.3.7 R2-2-1",
          "SA-MP 0.3.7 R2-1-1",
          "OPEN.MP v1.5.8.3079 (Static SSL)",
          "OPEN.MP v1.5.8.3079 (Dynamic SSL)",
          "OPEN.MP v1.4.0.2779 (Static SSL)",
          "OPEN.MP v1.4.0.2779 (Dynamic SSL)",
          "OPEN.MP v1.3.1.2748 (Static SSL)",
          "OPEN.MP v1.3.1.2748 (Dynamic SSL)",
          "OPEN.MP v1.2.0.2670 (Static SSL)",
          "OPEN.MP v1.2.0.2670 (Dynamic SSL)",
          "OPEN.MP v1.1.0.2612 (Static SSL)",
          "OPEN.MP v1.1.0.2612 (Dynamic SSL)"
      };

      const char keys[] = {
          'A','B','C','D','E','F','G','H','I','J','K','L','M','N'
      };

      char sel = select_interface("Select SA-MP / Open.MP Server", items, keys, 14);
      if (!sel) return 0;

      int idx = -1;
      if (sel >= 'A' && sel <= 'N') idx = sel - 'A';
      else if (sel >= 'a' && sel <= 'n') idx = sel - 'a';
      if (idx < 0 || idx >= 14) return 0;

      struct library_version_info versions[] = {
				/* SA-MP 0.3.DL R1 - First row version */
				{
						'A', "SA-MP 0.3.DL R1", /* Key A, display name */
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp03DLsvr_R1.tar.gz",
						"samp03DLsvr_R1.tar.gz", /* Linux archive */
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp03DL_svr_R1_win32.zip",
						"samp03DL_svr_R1_win32.zip" /* Windows archive */
				},
				/* SA-MP 0.3.7 R3 - Popular stable version */
				{
						'B', "SA-MP 0.3.7 R3",
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp037svr_R3.tar.gz",
						"samp037svr_R3.tar.gz",
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp037_svr_R3_win32.zip",
						"samp037_svr_R3_win32.zip"
				},
				/* SA-MP 0.3.7 R2-2-1 - Intermediate version */
				{
						'C', "SA-MP 0.3.7 R2-2-1",
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp037svr_R2-2-1.tar.gz",
						"samp037svr_R2-2-1.tar.gz",
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp037_svr_R2-1-1_win32.zip",
						"samp037_svr_R2-2-1_win32.zip"
				},
				/* SA-MP 0.3.7 R2-1-1 - Older stable version */
				{
						'D', "SA-MP 0.3.7 R2-1-1",
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp037svr_R2-1.tar.gz",
						"samp037svr_R2-1.tar.gz",
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp037_svr_R2-1-1_win32.zip",
						"samp037_svr_R2-1-1_win32.zip"
				},
				/* Open.MP v1.5.8.3079 - Latest Open.MP version (static SSL) */
				{
						'E', "OPEN.MP v1.5.8.3079 (Static SSL)",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.5.8.3079/open.mp-linux-x86.tar.gz",
						"open.mp-linux-x86.tar.gz", /* Linux Open.MP with static SSL */
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.5.8.3079/open.mp-win-x86.zip",
						"open.mp-win-x86.zip" /* Windows Open.MP */
				},
				/* Open.MP v1.5.8.3079 - Latest Open.MP version (dynamic SSL) */
				{
						'F', "OPEN.MP v1.5.8.3079 (Dynamic SSL)",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.5.8.3079/open.mp-linux-x86-dynssl.tar.gz",
						"open.mp-linux-x86-dynssl.tar.gz", /* Linux Open.MP with dynamic SSL */
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.5.8.3079/open.mp-win-x86.zip",
						"open.mp-win-x86.zip" /* Windows Open.MP (same as static) */
				},
				/* Open.MP v1.4.0.2779 - Previous Open.MP version (static SSL) */
				{
						'G', "OPEN.MP v1.4.0.2779 (Static SSL)",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.4.0.2779/open.mp-linux-x86.tar.gz",
						"open.mp-linux-x86.tar.gz", /* Linux Open.MP with static SSL */
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.4.0.2779/open.mp-win-x86.zip",
						"open.mp-win-x86.zip" /* Windows Open.MP */
				},
				/* Open.MP v1.4.0.2779 - Previous Open.MP version (dynamic SSL) */
				{
						'H', "OPEN.MP v1.4.0.2779 (Dynamic SSL)",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.4.0.2779/open.mp-linux-x86-dynssl.tar.gz",
						"open.mp-linux-x86-dynssl.tar.gz", /* Linux Open.MP with dynamic SSL */
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.4.0.2779/open.mp-win-x86.zip",
						"open.mp-win-x86.zip" /* Windows Open.MP (same as static) */
				},
				/* Open.MP v1.3.1.2748 - Previous Open.MP version (static SSL) */
				{
						'I', "OPEN.MP v1.3.1.2748 (Static SSL)",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.3.1.2748/open.mp-linux-x86.tar.gz",
						"open.mp-linux-x86.tar.gz",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.3.1.2748/open.mp-win-x86.zip",
						"open.mp-win-x86.zip"
				},
				/* Open.MP v1.3.1.2748 - Previous Open.MP version (dynamic SSL) */
				{
						'J', "OPEN.MP v1.3.1.2748 (Dynamic SSL)",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.3.1.2748/open.mp-linux-x86-dynssl.tar.gz",
						"open.mp-linux-x86-dynssl.tar.gz",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.3.1.2748/open.mp-win-x86.zip",
						"open.mp-win-x86.zip"
				},
				/* Open.MP v1.2.0.2670 - Older Open.MP version (static SSL) */
				{
						'K', "OPEN.MP v1.2.0.2670 (Static SSL)",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.2.0.2670/open.mp-linux-x86.tar.gz",
						"open.mp-linux-x86.tar.gz",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.2.0.2670/open.mp-win-x86.zip",
						"open.mp-win-x86.zip"
				},
				/* Open.MP v1.2.0.2670 - Older Open.MP version (dynamic SSL) */
				{
						'L', "OPEN.MP v1.2.0.2670 (Dynamic SSL)",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.2.0.2670/open.mp-linux-x86-dynssl.tar.gz",
						"open.mp-linux-x86-dynssl.tar.gz",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.2.0.2670/open.mp-win-x86.zip",
						"open.mp-win-x86.zip"
				},
				/* Open.MP v1.1.0.2612 - Early Open.MP version (static SSL) */
				{
						'M', "OPEN.MP v1.1.0.2612 (Static SSL)",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.1.0.2612/open.mp-linux-x86.tar.gz",
						"open.mp-linux-x86.tar.gz",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.1.0.2612/open.mp-win-x86.zip",
						"open.mp-win-x86.zip"
				},
				/* Open.MP v1.1.0.2612 - Early Open.MP version (dynamic SSL) */
				{
						'N', "OPEN.MP v1.1.0.2612 (Dynamic SSL)",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.1.0.2612/open.mp-linux-x86-dynssl.tar.gz",
						"open.mp-linux-x86-dynssl.tar.gz",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.1.0.2612/open.mp-win-x86.zip",
						"open.mp-win-x86.zip"
				}
      };

      struct library_version_info *chosen = &versions[idx];

      const char *url = (strcmp(platform, "linux") == 0) ?
        chosen->linux_url : chosen->windows_url;
      const char *filename = (strcmp(platform, "linux") == 0) ?
        chosen->linux_file : chosen->windows_file;

      wg_download_file(url, filename);
      return 0;
}
