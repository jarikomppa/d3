# DialogTree (D3)
Game dialog (as in, talking) engine

Copyright (c)2020 Jari Komppa
http://iki.fi/sol

## What is it

DialogTree is a work in progress scripting language and engine to facilitate various forms of dialogue in games. It can also be used, pretty much as is, as a visual novel engine, engine for multiple-choise adventures (just be careful not to use the chooseco trademarked name for these kinds of games), etc.

There are plans to make engines/intrepreters for c, c++, python and javascript at least.

Plans also exist to write proper documentation for it. We'll see how far my inspiration takes me.

## License

DialogTree engine along with its compiler are released under Unlicense, which makes them more or less public domain.

Feel free to contact me regarding it, but no promises.

## Background

This is my third approach to an engine like this. My first one went too far with some things and not far enough in others. The second one I made for the ZX Spectrum, which imposed limitations on the design, which meant I had to concentrate on what's actually needed. This third approach takes the ZX Spectrum engine and removes all hardware limitations and ZX Spectrum specific things.

## Basic Idea

Think of a discussion as a stack of cards. Each card has a bunch of text, and a few possible responses. Each response then says which card to move to. 

The script looks something like:
```
$Q welcome
Welcome to my ted talk.
$A flee
Flee from here
$A continue
Do continue

$Q flee
You hastily vacate the premises

$Q continue
As I said, welcome.
$A continue
yeesss?
$A flee
Decide to flee after all
```
This is then fed through the compiler, which currently only supports json output, but is meant to also support custom binary chunked format. The resulting file is fed to an engine which can then be queried for the state of the discussion. A simple python usage might look something like:
```python
import d3
d = d3()
d.load_json(sys.argv[1])
while True:
    print(d.text) # <- current "room" description
    print("-   -  - ----- -  -   -")
    for i in range(len(d.answers)):
        print(i, ")", d.answers[i]) # <- current "room" choises
    print("q ) quit")
    ok = False
    while ok is False:
        i = input(">")
        if i == "q":
            quit()
        valid = False
        intv = 0
        try:
            intv = int(i)
            valid = True
        except ValueError:
            print("Invalid option")
        if valid:
            if intv < 0 or intv > len(d.answers) - 1:
                print("Choice out of range,", intv, "not within 0 ..", len(d.answers) - 1)
            else:
                d.choose(i) # <- moving to the next "room"
                ok = True
```
..and most of the complexity there goes into making sure python doesn't crash when you typo the input.

Something similar in C:
```C
void * s = d3state_alloc();
d = d3_alloc(s);
d3_loadfile(d, pars[1]);
while (1)
{ 
    printf("%s\n", d->mText);
    printf("-   -  - ---- -  -   -\n");
    for (i = 0; i < d->mAnswerCount; i++)
        printf("%d) %s\n", i, d->mAnswer[i]);
    printf("q) quit\n");
    ok = 0;
    while (!ok)
    {
        c = _getch();
        if (c == 'q')
        {
            d3_free(d);
            d3state_free(s);
            return 0;
        }
        c = c - '0';
        if (c < 0 || c > d->mAnswerCount - 1)
        {
            printf("Choice out of range, 0 .. %d expected\n", d->mAnswerCount - 1);
        }
        else
        {
            ok = 1;
            d3_choose(d, c);
        }
    }
}
```

Now, if this was all DialogTree did, it wouldn't really need much of an engine. In addition to the moving from one card to another bit, DialogTree has quite bit of specialized, easy to use logic built in, meaning you can make it optionally to give certain options or display certain text. It can do bunch of boolean operations as well as handle simple math operations and comparisons.

More about those in the documentation, when I get to that.