// SPDX-FileCopyrightText: Copyright (c) 2023 merryhime <https://mary.rs>
// SPDX-License-Identifier: MIT

void FJCVTZS(WReg wd, DReg rn)
{
    emit<"0001111001111110000000nnnnnddddd", "d", "n">(wd, rn);
}