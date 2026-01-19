
Copyright (C) 2025 HTotoo


# External UI elements

  
## Read carefully
  

These widgets are only used in external apps! 
The concept is, we move these widgets out of FW space, and compile them with external apps. To all separately. This is multiple include, but since we compile the widget under different namespaces (and those namespaces will be under the external_app namespace) these all will be removed from the base firmware.

This way we can free up some spaces, and still reuse the same widgets in different apps easily.

## How to create external widget

You create a **hpp** file indet the ui/external folder, and include everything you need for your widget as you normally wanted to do. 
**Don't forget the ifdef guards!** 
**Never use any namespace for your class there!** The namespace of the actual ext app will be used where you include this.

Then create a **cpi** file, where you include your hpp file, and create the implementation of your widget class. Still, don't use namespace.

You won't need to include these files in any cmake file! (see usage, why)

## How to use these widgets

This is important, so follow the rules carefully. If it works, doesn't mean, you did it good!
In your external app's hpp file, you need to include the widget's hpp file. 
**But be careful**, you must include it under the ext app's namespace, before your first class. For examlpe:

    namespace  ui::external_app::gfxeq {
    #include  "external/ui_grapheq.hpp"
    class  gfxEQView : public  View {
Then you must include the cpi file in the external app's cpp file. Again, it must be under the ext app's namespace!

    using  namespace  portapack;
    namespace  ui::external_app::gfxeq {
    #include  "external/ui_grapheq.cpi"
    gfxEQView::gfxEQView(NavigationView&  nav)

This must be done under each ext app that uses the same widget.
This way, the widget will be compiled multiple times under the external_app namespace, but those will be removed from the base. Each app will be able to use it, easy to handle, easy to create.