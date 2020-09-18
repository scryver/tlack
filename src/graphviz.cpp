internal void
graph_connect(FileStream *output, String from, String to)
{
    println(output, "%.*s -> %.*s;", STR_FMT(from), STR_FMT(to));
}

internal void
graph_dash_connect(FileStream *output, String from, String to)
{
    println(output, "%.*s -> %.*s [style=dashed];", STR_FMT(from), STR_FMT(to));
}

internal void
graph_label(FileStream *output, String name, String label)
{
    println(output, "%.*s [label=\"%.*s\"];", STR_FMT(name), STR_FMT(label));
}

internal void
graph_label(FileStream *output, String name, char *labelFmt, ...)
{
    println_begin(output, "%.*s [label=\"", STR_FMT(name));
    
    va_list args;
    va_start(args, labelFmt);
    vprint(output, labelFmt, args);
    va_end(args);
    
    println_end(output, "\"];");
}
