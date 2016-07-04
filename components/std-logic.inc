system buffer(bool o, i)
{
    o = i;
}

system inverter(bool o, i)
{
    o = !i;
}

system xor2(bool o, i1, i2)
{
    o = (i1 != i2);
}

system and2(bool o, i1, i2)
{
    o = (i1 and i2);
}

system and3(bool o, i1, i2, i3)
{
    o = (i1 and i2 and i3);
}

system and4(bool o, i1, i2, i3, i4)
{
    o = (i1 and i2 and i3 and i4);
}

system and5(bool o, i1, i2, i3, i4, i5)
{
    o = (i1 and i2 and i3 and i4 and i5);
}

system and6(bool o, i1, i2, i3, i4, i5, i6)
{
    o = (i1 and i2 and i3 and i4 and i5 and i6);
}

system and7(bool o, i1, i2, i3, i4, i5, i6, i7)
{
    o = (i1 and i2 and i3 and i4 and i5 and i6 and i7);
}

system and8(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
    o = (i1 and i2 and i3 and i4 and i5 and i6 and i7 and i8);
}

system and9(bool o, i1, i2, i3, i4, i5, i6, i7, i8, i9)
{
    o = (i1 and i2 and i3 and i4 and i5 and i6 and i7 and i8 and i9);
}

system nand2(bool o, i1, i2)
{
    o = !(i1 and i2);
}

system nand3(bool o, i1, i2, i3)
{
    o = !(i1 and i2 and i3);
}

system nand4(bool o, i1, i2, i3, i4)
{
    o = !(i1 and i2 and i3 and i4);
}

system nand5(bool o, i1, i2, i3, i4, i5)
{
    o = !(i1 and i2 and i3 and i4 and i5);
}

system nand6(bool o, i1, i2, i3, i4, i5, i6)
{
    o = !(i1 and i2 and i3 and i4 and i5 and i6);
}

system nand7(bool o, i1, i2, i3, i4, i5, i6, i7)
{
    o = !(i1 and i2 and i3 and i4 and i5 and i6 and i7);
}

system nand8(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
    o = !(i1 and i2 and i3 and i4 and i5 and i6 and i7 and i8);
}

system or2(bool o, i1, i2)
{
    o = (i1 or i2);
}

system or3(bool o, i1, i2, i3)
{
    o = (i1 or i2 or i3);
}

system or4(bool o, i1, i2, i3, i4)
{
    o = (i1 or i2 or i3 or i4);
}

system or5(bool o, i1, i2, i3, i4, i5)
{
    o = (i1 or i2 or i3 or i4 or i5);
}

system or6(bool o, i1, i2, i3, i4, i5, i6)
{
    o = (i1 or i2 or i3 or i4 or i5 or i6);
}

system or7(bool o, i1, i2, i3, i4, i5, i6, i7)
{
    o = (i1 or i2 or i3 or i4 or i5 or i6 or i7);
}

system or8(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
    o = (i1 or i2 or i3 or i4 or i5 or i6 or i7 or i8);
}

system nor2(bool o, i1, i2)
{
    o = !(i1 or i2);
}

system nor3(bool o, i1, i2, i3)
{
    o = !(i1 or i2 or i3);
}

system nor4(bool o, i1, i2, i3, i4)
{
    o = !(i1 or i2 or i3 or i4);
}

system nor5(bool o, i1, i2, i3, i4, i5)
{
    o = !(i1 or i2 or i3 or i4 or i5);
}

system nor6(bool o, i1, i2, i3, i4, i5, i6)
{
    o = !(i1 or i2 or i3 or i4 or i5 or i6);
}

system nor7(bool o, i1, i2, i3, i4, i5, i6, i7)
{
    o = !(i1 or i2 or i3 or i4 or i5 or i6 or i7);
}

system nor8(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
    o = !(i1 or i2 or i3 or i4 or i5 or i6 or i7 or i8);
}

system nor9(bool o, i1, i2, i3, i4, i5, i6, i7, i8, i9)
{
    o = !(i1 or i2 or i3 or i4 or i5 or i6 or i7 or i8 or i9);
}

system halfadder2(bool i1, i2, sum, carry)
{
    system and2 A(carry, i1, i2);
    system xor2 X(sum, i1, i2);
}

system fulladder2(bool i1, i2, ci, sum, carry)
{
    bool f, p, q;

    system halfadder2 HA1(i1, i2, f, p);
    system halfadder2 HA2(ci, f, sum, q);
    system or2 O(carry, p, q);
}
