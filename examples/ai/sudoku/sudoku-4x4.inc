type tile = enum { one, two, three, four };

system Everywhere(tile g[4])
{
	forall (i in 0 .. 3) {
		forall (j in i + 1 .. 3) {
			g[i] != g[j];
		}
	}
}

system Grid(tile g[4][4])
{
	system Everywhere h[4], v[4], b[4];

	forall (i in 0 .. 3) {
		h[i]([ g[i][0], g[i][1], g[i][2], g[i][3] ]);
		v[i]([ g[0][i], g[1][i], g[2][i], g[3][i] ]);
	}

	b[0]([ g[0][0], g[0][1], g[1][0], g[1][1] ]);
	b[1]([ g[0][2], g[0][3], g[1][2], g[1][3] ]);
	b[2]([ g[2][0], g[2][1], g[3][0], g[3][1] ]);
	b[3]([ g[2][2], g[2][3], g[3][2], g[3][3] ]);
}
