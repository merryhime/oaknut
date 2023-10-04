// SPDX-FileCopyrightText: Copyright (c) 2023 merryhime <https://mary.rs>
// SPDX-License-Identifier: MIT

void LDAPR(WReg wt, XRegSp xn)
{
    emit<"1011100010111111110000nnnnnttttt", "t", "n">(wt, xn);
}
void LDAPR(XReg xt, XRegSp xn)
{
    emit<"1111100010111111110000nnnnnttttt", "t", "n">(xt, xn);
}
void LDAPRB(WReg wt, XRegSp xn)
{
    emit<"0011100010111111110000nnnnnttttt", "t", "n">(wt, xn);
}
void LDAPRH(WReg wt, XRegSp xn)
{
    emit<"0111100010111111110000nnnnnttttt", "t", "n">(wt, xn);
}