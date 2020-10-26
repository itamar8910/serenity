#include "lib.h"
#include <LibCore/Command.h>
#include <LibGUI/Application.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Button.h>
#include <LibGUI/Label.h>
#include <LibGUI/Widget.h>
#include <LibGUI/Window.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/internals.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char** argv, char** env)
{
    (void)argc;
    (void)argv;
    (void)env;

    printf("Well Hello Friends!\n");
    printf("trying to open /etc/fstab for writing..\n");
    int rc = open("/etc/fstab", O_RDWR);
    if (rc == -1) {
        int _errno = errno;
        perror("open failed");
        printf("rc: %d, errno: %d\n", rc, _errno);
    }
    printf("ls: %s\n", Core::command("ls", {}).characters());
    auto app = GUI::Application::construct(argc, argv);

    auto window = GUI::Window::construct();
    window->resize(240, 160);
    window->set_title("Hello World!");
    window->set_icon(Gfx::Bitmap::load_from_file("/res/icons/16x16/app-hello-world.png"));

    auto& main_widget = window->set_main_widget<GUI::Widget>();
    main_widget.set_fill_with_background_color(true);
    main_widget.set_background_color(Color::White);
    auto& layout = main_widget.set_layout<GUI::VerticalBoxLayout>();
    layout.set_margins({ 4, 4, 4, 4 });

    auto& label = main_widget.add<GUI::Label>();
    label.set_text("Hello\nWorld!");

    auto& button = main_widget.add<GUI::Button>();
    button.set_text("Good-bye");
    button.set_size_policy(GUI::SizePolicy::Fill, GUI::SizePolicy::Fixed);
    button.set_preferred_size(0, 20);
    button.on_click = [&](auto) {
        app->quit();
    };

    window->show();

    return app->exec();
    // return func() + g_tls1 + g_tls2;
}
