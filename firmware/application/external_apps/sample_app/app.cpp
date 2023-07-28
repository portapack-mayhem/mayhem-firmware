/* Sample PPA */

using fn = int(*)();

int app_start(fn f) {
    return f();
}
