#include "ui_about_simple.hpp"

namespace ui {
AboutView::AboutView(NavigationView& nav) {
    add_children({&console, &button_ok});

    button_ok.on_select = [&nav](Button&) {
        nav.pop();
    };

    console.writeln(STR_COLOR_LIGHT_GREY "List of contributors:");
    console.writeln("");
}

void AboutView::update() {
    if (++timer > 200) {
        timer = 0;

        switch (++frame) {
            case 1:
                // TODO: Generate this automatically from github
                // https://github.com/eried/portapack-mayhem/graphs/contributors?to=2022-01-01&from=2020-04-12&type=c
                console.writeln(STR_COLOR_DARK_YELLOW "Mayhem:");
                console.writeln("eried,euquiq,gregoryfenton");
                console.writeln("johnelder,jwetzell,nnemanjan00");
                console.writeln("N0vaPixel,klockee,GullCode");
                console.writeln("jamesshao8,ITAxReal,rascafr");
                console.writeln("mcules,dqs105,strijar");
                console.writeln("zhang00963,RedFox-Fr,aldude999");
                console.writeln("East2West,fossum,ArjanOnwezen");
                console.writeln("vXxOinvizioNxX,teixeluis");
                console.writeln("Brumi-2021,texasyojimbo");
                console.writeln("heurist1,intoxsick,ckuethe");
                console.writeln("notpike,jLynx,zigad");
                console.writeln("MichalLeonBorsuk,jimilinuxguy");
                console.writeln("kallanreed,bernd-herzog");
                break;
            case 2:
                console.writeln("NotherNgineer,zxkmm,u-foka");
                console.writeln("");
                break;

            case 3:
                // https://github.com/eried/portapack-mayhem/graphs/contributors?to=2020-04-12&from=2015-07-31&type=c
                console.writeln(STR_COLOR_DARK_YELLOW "Havoc:");
                console.writeln("furrtek,mrmookie,NotPike");
                console.writeln("mjwaxios,ImDroided,Giorgiofox");
                console.writeln("F4GEV,z4ziggy,xmycroftx");
                console.writeln("troussos,silascutler");
                console.writeln("nickbouwhuis,msoose,leres");
                console.writeln("joakar,dhoetger,clem-42");
                console.writeln("brianlechthaler,ZeroChaos-...");
                console.writeln("");
                break;

            case 4:
                // https://github.com/eried/portapack-mayhem/graphs/contributors?from=2014-07-05&to=2015-07-31&type=c
                console.writeln(STR_COLOR_DARK_YELLOW "PortaPack:");
                console.writeln("jboone,argilo");
                console.writeln("");
                break;

            case 5:
                // https://github.com/mossmann/hackrf/graphs/contributors
                console.writeln(STR_COLOR_DARK_YELLOW "HackRF:");
                console.writeln("mossmann,dominicgs,bvernoux");
                console.writeln("bgamari,schneider42,miek");
                console.writeln("willcode,hessu,Sec42");
                console.writeln("yhetti,ckuethe,smunaut");
                console.writeln("wishi,mrbubble62,scateu...");
                console.writeln("");
                frame = 0;  // Loop
                break;
        }
    }
}

void AboutView::focus() {
    button_ok.focus();
}

} /* namespace ui */
