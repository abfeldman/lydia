system buffer(bool o, i)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = i);
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system inverter(bool o, i)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;

	(!f0 and !f1) => (o = !i);
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system xor2(bool o, i1, i2)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 != i2));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system and2(bool o, i1, i2)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 and i2));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system and3(bool o, i1, i2, i3)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 and i2 and i3));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system and4(bool o, i1, i2, i3, i4)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 and i2 and i3 and i4));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system and5(bool o, i1, i2, i3, i4, i5)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 and i2 and i3 and i4 and i5));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system and6(bool o, i1, i2, i3, i4, i5, i6)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 and i2 and i3 and i4 and i5 and i6));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system and7(bool o, i1, i2, i3, i4, i5, i6, i7)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 and i2 and i3 and i4 and i5 and i6 and i7));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system and8(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 and i2 and i3 and i4 and i5 and i6 and i7 and i8));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system and9(bool o, i1, i2, i3, i4, i5, i6, i7, i8, i9)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 and i2 and i3 and i4 and i5 and i6 and i7 and i8 and i9));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system nand2(bool o, i1, i2)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = !(i1 and i2));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system nand3(bool o, i1, i2, i3)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = !(i1 and i2 and i3));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system nand4(bool o, i1, i2, i3, i4)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = !(i1 and i2 and i3 and i4));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system nand5(bool o, i1, i2, i3, i4, i5)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = !(i1 and i2 and i3 and i4 and i5));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system nand6(bool o, i1, i2, i3, i4, i5, i6)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = !(i1 and i2 and i3 and i4 and i5 and i6));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system nand7(bool o, i1, i2, i3, i4, i5, i6, i7)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = !(i1 and i2 and i3 and i4 and i5 and i6 and i7));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system nand8(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = !(i1 and i2 and i3 and i4 and i5 and i6 and i7 and i8));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system or2(bool o, i1, i2)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 or i2));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system or3(bool o, i1, i2, i3)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 or i2 or i3));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system or4(bool o, i1, i2, i3, i4)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 or i2 or i3 or i4));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system or5(bool o, i1, i2, i3, i4, i5)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 or i2 or i3 or i4 or i5));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system or6(bool o, i1, i2, i3, i4, i5, i6)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 or i2 or i3 or i4 or i5 or i6));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system or7(bool o, i1, i2, i3, i4, i5, i6, i7)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 or i2 or i3 or i4 or i5 or i6 or i7));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system or8(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = (i1 or i2 or i3 or i4 or i5 or i6 or i7 or i8));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system nor2(bool o, i1, i2)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = !(i1 or i2));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system nor3(bool o, i1, i2, i3)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = !(i1 or i2 or i3));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system nor4(bool o, i1, i2, i3, i4)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = !(i1 or i2 or i3 or i4));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system nor5(bool o, i1, i2, i3, i4, i5)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = !(i1 or i2 or i3 or i4 or i5));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system nor6(bool o, i1, i2, i3, i4, i5, i6)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = !(i1 or i2 or i3 or i4 or i5 or i6));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system nor7(bool o, i1, i2, i3, i4, i5, i6, i7)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = !(i1 or i2 or i3 or i4 or i5 or i6 or i7));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system nor8(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = !(i1 or i2 or i3 or i4 or i5 or i6 or i7 or i8));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
}

system nor9(bool o, i1, i2, i3, i4, i5, i6, i7, i8, i9)
{
	bool f0, f1;

	attribute probability(f0, f1) = \f (f ? 0.01 : 0.99);
	attribute health(f0, f1) = \f !f;
	
	(!f0 and !f1) => (o = !(i1 or i2 or i3 or i4 or i5 or i6 or i7 or i8 or i9));
	f0 => (o = false);
	f1 => (o = true);
	!(f0 and f1);
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
