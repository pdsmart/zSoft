-- ZPU
--
-- Copyright 2004-2008 oharboe - �yvind Harboe - oyvind.harboe@zylin.com
-- Modified by Philip Smart 02/2019 for the ZPU Evo implementation.
--
-- The FreeBSD license
-- 
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions
-- are met:
-- 
-- 1. Redistributions of source code must retain the above copyright
--    notice, this list of conditions and the following disclaimer.
-- 2. Redistributions in binary form must reproduce the above
--    copyright notice, this list of conditions and the following
--    disclaimer in the documentation and/or other materials
--    provided with the distribution.
-- 
-- THIS SOFTWARE IS PROVIDED BY THE ZPU PROJECT ``AS IS'' AND ANY
-- EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
-- THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
-- PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
-- ZPU PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
-- INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
-- (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
-- OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
-- HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
-- STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
-- ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-- 
-- The views and conclusions contained in the software and documentation
-- are those of the authors and should not be interpreted as representing
-- official policies, either expressed or implied, of the ZPU Project.

library ieee;
library pkgs;
library work;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.zpu_pkg.all;
use work.zpu_soc_pkg.all;

entity BootROM is
port (
        clk                  : in  std_logic;
        areset               : in  std_logic := '0';
        memAWriteEnable      : in  std_logic;
        memAAddr             : in  std_logic_vector(ADDR_32BIT_BRAM_RANGE);
        memAWrite            : in  std_logic_vector(WORD_32BIT_RANGE);
        memBWriteEnable      : in  std_logic;
        memBAddr             : in  std_logic_vector(ADDR_32BIT_BRAM_RANGE);
        memBWrite            : in  std_logic_vector(WORD_32BIT_RANGE);
        memARead             : out std_logic_vector(WORD_32BIT_RANGE);
        memBRead             : out std_logic_vector(WORD_32BIT_RANGE)
);
end BootROM;

architecture arch of BootROM is

type ram_type is array(natural range 0 to (2**(SOC_MAX_ADDR_BRAM_BIT-2))-1) of std_logic_vector(WORD_32BIT_RANGE);

shared variable ram : ram_type :=
(
             0 => x"0b0b0b88",
             1 => x"e0040000",
             2 => x"00000000",
             3 => x"00000000",
             4 => x"00000000",
             5 => x"00000000",
             6 => x"00000000",
             7 => x"00000000",
             8 => x"88088c08",
             9 => x"90080b0b",
            10 => x"0b888008",
            11 => x"2d900c8c",
            12 => x"0c880c04",
            13 => x"00000000",
            14 => x"00000000",
            15 => x"00000000",
            16 => x"71fd0608",
            17 => x"72830609",
            18 => x"81058205",
            19 => x"832b2a83",
            20 => x"ffff0652",
            21 => x"04000000",
            22 => x"00000000",
            23 => x"00000000",
            24 => x"71fd0608",
            25 => x"83ffff73",
            26 => x"83060981",
            27 => x"05820583",
            28 => x"2b2b0906",
            29 => x"7383ffff",
            30 => x"0b0b0b0b",
            31 => x"83a50400",
            32 => x"72098105",
            33 => x"72057373",
            34 => x"09060906",
            35 => x"73097306",
            36 => x"070a8106",
            37 => x"53510400",
            38 => x"00000000",
            39 => x"00000000",
            40 => x"72722473",
            41 => x"732e0753",
            42 => x"51040000",
            43 => x"00000000",
            44 => x"00000000",
            45 => x"00000000",
            46 => x"00000000",
            47 => x"00000000",
            48 => x"71737109",
            49 => x"71068106",
            50 => x"09810572",
            51 => x"0a100a72",
            52 => x"0a100a31",
            53 => x"050a8106",
            54 => x"51515351",
            55 => x"04000000",
            56 => x"72722673",
            57 => x"732e0753",
            58 => x"51040000",
            59 => x"00000000",
            60 => x"00000000",
            61 => x"00000000",
            62 => x"00000000",
            63 => x"00000000",
            64 => x"00000000",
            65 => x"00000000",
            66 => x"00000000",
            67 => x"00000000",
            68 => x"00000000",
            69 => x"00000000",
            70 => x"00000000",
            71 => x"00000000",
            72 => x"0b0b0b88",
            73 => x"c4040000",
            74 => x"00000000",
            75 => x"00000000",
            76 => x"00000000",
            77 => x"00000000",
            78 => x"00000000",
            79 => x"00000000",
            80 => x"720a722b",
            81 => x"0a535104",
            82 => x"00000000",
            83 => x"00000000",
            84 => x"00000000",
            85 => x"00000000",
            86 => x"00000000",
            87 => x"00000000",
            88 => x"72729f06",
            89 => x"0981050b",
            90 => x"0b0b88a7",
            91 => x"05040000",
            92 => x"00000000",
            93 => x"00000000",
            94 => x"00000000",
            95 => x"00000000",
            96 => x"72722aff",
            97 => x"739f062a",
            98 => x"0974090a",
            99 => x"8106ff05",
           100 => x"06075351",
           101 => x"04000000",
           102 => x"00000000",
           103 => x"00000000",
           104 => x"71715351",
           105 => x"04067383",
           106 => x"06098105",
           107 => x"8205832b",
           108 => x"0b2b0772",
           109 => x"fc060c51",
           110 => x"51040000",
           111 => x"00000000",
           112 => x"72098105",
           113 => x"72050970",
           114 => x"81050906",
           115 => x"0a810653",
           116 => x"51040000",
           117 => x"00000000",
           118 => x"00000000",
           119 => x"00000000",
           120 => x"72098105",
           121 => x"72050970",
           122 => x"81050906",
           123 => x"0a098106",
           124 => x"53510400",
           125 => x"00000000",
           126 => x"00000000",
           127 => x"00000000",
           128 => x"71098105",
           129 => x"52040000",
           130 => x"00000000",
           131 => x"00000000",
           132 => x"00000000",
           133 => x"00000000",
           134 => x"00000000",
           135 => x"00000000",
           136 => x"72720981",
           137 => x"05055351",
           138 => x"04000000",
           139 => x"00000000",
           140 => x"00000000",
           141 => x"00000000",
           142 => x"00000000",
           143 => x"00000000",
           144 => x"72097206",
           145 => x"73730906",
           146 => x"07535104",
           147 => x"00000000",
           148 => x"00000000",
           149 => x"00000000",
           150 => x"00000000",
           151 => x"00000000",
           152 => x"71fc0608",
           153 => x"72830609",
           154 => x"81058305",
           155 => x"1010102a",
           156 => x"81ff0652",
           157 => x"04000000",
           158 => x"00000000",
           159 => x"00000000",
           160 => x"71fc0608",
           161 => x"0b0b0b9f",
           162 => x"d0738306",
           163 => x"10100508",
           164 => x"060b0b0b",
           165 => x"88ac0400",
           166 => x"00000000",
           167 => x"00000000",
           168 => x"88088c08",
           169 => x"90087575",
           170 => x"0b0b0b89",
           171 => x"cf2d5050",
           172 => x"88085690",
           173 => x"0c8c0c88",
           174 => x"0c510400",
           175 => x"00000000",
           176 => x"88088c08",
           177 => x"90087575",
           178 => x"0b0b0b8b",
           179 => x"812d5050",
           180 => x"88085690",
           181 => x"0c8c0c88",
           182 => x"0c510400",
           183 => x"00000000",
           184 => x"72097081",
           185 => x"0509060a",
           186 => x"8106ff05",
           187 => x"70547106",
           188 => x"73097274",
           189 => x"05ff0506",
           190 => x"07515151",
           191 => x"04000000",
           192 => x"72097081",
           193 => x"0509060a",
           194 => x"098106ff",
           195 => x"05705471",
           196 => x"06730972",
           197 => x"7405ff05",
           198 => x"06075151",
           199 => x"51040000",
           200 => x"05ff0504",
           201 => x"00000000",
           202 => x"00000000",
           203 => x"00000000",
           204 => x"00000000",
           205 => x"00000000",
           206 => x"00000000",
           207 => x"00000000",
           208 => x"04000000",
           209 => x"00000000",
           210 => x"00000000",
           211 => x"00000000",
           212 => x"00000000",
           213 => x"00000000",
           214 => x"00000000",
           215 => x"00000000",
           216 => x"71810552",
           217 => x"04000000",
           218 => x"00000000",
           219 => x"00000000",
           220 => x"00000000",
           221 => x"00000000",
           222 => x"00000000",
           223 => x"00000000",
           224 => x"04000000",
           225 => x"00000000",
           226 => x"00000000",
           227 => x"00000000",
           228 => x"00000000",
           229 => x"00000000",
           230 => x"00000000",
           231 => x"00000000",
           232 => x"02840572",
           233 => x"10100552",
           234 => x"04000000",
           235 => x"00000000",
           236 => x"00000000",
           237 => x"00000000",
           238 => x"00000000",
           239 => x"00000000",
           240 => x"00000000",
           241 => x"00000000",
           242 => x"00000000",
           243 => x"00000000",
           244 => x"00000000",
           245 => x"00000000",
           246 => x"00000000",
           247 => x"00000000",
           248 => x"717105ff",
           249 => x"05715351",
           250 => x"020d0400",
           251 => x"00000000",
           252 => x"00000000",
           253 => x"00000000",
           254 => x"00000000",
           255 => x"00000000",
           256 => x"00000404",
           257 => x"04000000",
           258 => x"10101010",
           259 => x"10101010",
           260 => x"10101010",
           261 => x"10101010",
           262 => x"10101010",
           263 => x"10101010",
           264 => x"10101010",
           265 => x"10101053",
           266 => x"51040000",
           267 => x"7381ff06",
           268 => x"73830609",
           269 => x"81058305",
           270 => x"1010102b",
           271 => x"0772fc06",
           272 => x"0c515104",
           273 => x"72728072",
           274 => x"8106ff05",
           275 => x"09720605",
           276 => x"71105272",
           277 => x"0a100a53",
           278 => x"72ed3851",
           279 => x"51535104",
           280 => x"9ff470a0",
           281 => x"a4278e38",
           282 => x"80717084",
           283 => x"05530c0b",
           284 => x"0b0b88e2",
           285 => x"0488fe51",
           286 => x"9e9d0400",
           287 => x"80040088",
           288 => x"fe040000",
           289 => x"00940802",
           290 => x"940cfd3d",
           291 => x"0d805394",
           292 => x"088c0508",
           293 => x"52940888",
           294 => x"05085182",
           295 => x"de3f8808",
           296 => x"70880c54",
           297 => x"853d0d94",
           298 => x"0c049408",
           299 => x"02940cfd",
           300 => x"3d0d8153",
           301 => x"94088c05",
           302 => x"08529408",
           303 => x"88050851",
           304 => x"82b93f88",
           305 => x"0870880c",
           306 => x"54853d0d",
           307 => x"940c0494",
           308 => x"0802940c",
           309 => x"f93d0d80",
           310 => x"0b9408fc",
           311 => x"050c9408",
           312 => x"88050880",
           313 => x"25ab3894",
           314 => x"08880508",
           315 => x"30940888",
           316 => x"050c800b",
           317 => x"9408f405",
           318 => x"0c9408fc",
           319 => x"05088838",
           320 => x"810b9408",
           321 => x"f4050c94",
           322 => x"08f40508",
           323 => x"9408fc05",
           324 => x"0c94088c",
           325 => x"05088025",
           326 => x"ab389408",
           327 => x"8c050830",
           328 => x"94088c05",
           329 => x"0c800b94",
           330 => x"08f0050c",
           331 => x"9408fc05",
           332 => x"08883881",
           333 => x"0b9408f0",
           334 => x"050c9408",
           335 => x"f0050894",
           336 => x"08fc050c",
           337 => x"80539408",
           338 => x"8c050852",
           339 => x"94088805",
           340 => x"085181a7",
           341 => x"3f880870",
           342 => x"9408f805",
           343 => x"0c549408",
           344 => x"fc050880",
           345 => x"2e8c3894",
           346 => x"08f80508",
           347 => x"309408f8",
           348 => x"050c9408",
           349 => x"f8050870",
           350 => x"880c5489",
           351 => x"3d0d940c",
           352 => x"04940802",
           353 => x"940cfb3d",
           354 => x"0d800b94",
           355 => x"08fc050c",
           356 => x"94088805",
           357 => x"08802593",
           358 => x"38940888",
           359 => x"05083094",
           360 => x"0888050c",
           361 => x"810b9408",
           362 => x"fc050c94",
           363 => x"088c0508",
           364 => x"80258c38",
           365 => x"94088c05",
           366 => x"08309408",
           367 => x"8c050c81",
           368 => x"5394088c",
           369 => x"05085294",
           370 => x"08880508",
           371 => x"51ad3f88",
           372 => x"08709408",
           373 => x"f8050c54",
           374 => x"9408fc05",
           375 => x"08802e8c",
           376 => x"389408f8",
           377 => x"05083094",
           378 => x"08f8050c",
           379 => x"9408f805",
           380 => x"0870880c",
           381 => x"54873d0d",
           382 => x"940c0494",
           383 => x"0802940c",
           384 => x"fd3d0d81",
           385 => x"0b9408fc",
           386 => x"050c800b",
           387 => x"9408f805",
           388 => x"0c94088c",
           389 => x"05089408",
           390 => x"88050827",
           391 => x"ac389408",
           392 => x"fc050880",
           393 => x"2ea33880",
           394 => x"0b94088c",
           395 => x"05082499",
           396 => x"3894088c",
           397 => x"05081094",
           398 => x"088c050c",
           399 => x"9408fc05",
           400 => x"08109408",
           401 => x"fc050cc9",
           402 => x"399408fc",
           403 => x"0508802e",
           404 => x"80c93894",
           405 => x"088c0508",
           406 => x"94088805",
           407 => x"0826a138",
           408 => x"94088805",
           409 => x"0894088c",
           410 => x"05083194",
           411 => x"0888050c",
           412 => x"9408f805",
           413 => x"089408fc",
           414 => x"05080794",
           415 => x"08f8050c",
           416 => x"9408fc05",
           417 => x"08812a94",
           418 => x"08fc050c",
           419 => x"94088c05",
           420 => x"08812a94",
           421 => x"088c050c",
           422 => x"ffaf3994",
           423 => x"08900508",
           424 => x"802e8f38",
           425 => x"94088805",
           426 => x"08709408",
           427 => x"f4050c51",
           428 => x"8d399408",
           429 => x"f8050870",
           430 => x"9408f405",
           431 => x"0c519408",
           432 => x"f4050888",
           433 => x"0c853d0d",
           434 => x"940c04ff",
           435 => x"3d0d8188",
           436 => x"0b87c092",
           437 => x"8c0c810b",
           438 => x"87c0928c",
           439 => x"0c850b87",
           440 => x"c0988c0c",
           441 => x"87c0928c",
           442 => x"08708206",
           443 => x"51517080",
           444 => x"2e8a3887",
           445 => x"c0988c08",
           446 => x"5170e938",
           447 => x"87c0928c",
           448 => x"08fc8080",
           449 => x"06527193",
           450 => x"3887c098",
           451 => x"8c085170",
           452 => x"802e8838",
           453 => x"710b0b0b",
           454 => x"9ff0340b",
           455 => x"0b0b9ff0",
           456 => x"33880c83",
           457 => x"3d0d04fa",
           458 => x"3d0d787b",
           459 => x"7d565856",
           460 => x"800b0b0b",
           461 => x"0b9ff033",
           462 => x"81065255",
           463 => x"82527075",
           464 => x"2e098106",
           465 => x"819e3885",
           466 => x"0b87c098",
           467 => x"8c0c7987",
           468 => x"c092800c",
           469 => x"840b87c0",
           470 => x"928c0c87",
           471 => x"c0928c08",
           472 => x"70852a70",
           473 => x"81065152",
           474 => x"5370802e",
           475 => x"a73887c0",
           476 => x"92840870",
           477 => x"81ff0676",
           478 => x"79275253",
           479 => x"5173802e",
           480 => x"90387080",
           481 => x"2e8b3871",
           482 => x"76708105",
           483 => x"5834ff14",
           484 => x"54811555",
           485 => x"72a20651",
           486 => x"70802e8b",
           487 => x"3887c098",
           488 => x"8c085170",
           489 => x"ffb53887",
           490 => x"c0988c08",
           491 => x"51709538",
           492 => x"810b87c0",
           493 => x"928c0c87",
           494 => x"c0928c08",
           495 => x"70820651",
           496 => x"5170f438",
           497 => x"8073fc80",
           498 => x"80065252",
           499 => x"70722e09",
           500 => x"81068f38",
           501 => x"87c0988c",
           502 => x"08517072",
           503 => x"2e098106",
           504 => x"83388152",
           505 => x"71880c88",
           506 => x"3d0d04fe",
           507 => x"3d0d7481",
           508 => x"11337133",
           509 => x"71882b07",
           510 => x"880c5351",
           511 => x"843d0d04",
           512 => x"fd3d0d75",
           513 => x"83113382",
           514 => x"12337190",
           515 => x"2b71882b",
           516 => x"07811433",
           517 => x"70720788",
           518 => x"2b753371",
           519 => x"07880c52",
           520 => x"53545654",
           521 => x"52853d0d",
           522 => x"04f93d0d",
           523 => x"790b0b0b",
           524 => x"9ff40857",
           525 => x"57817727",
           526 => x"80ed3876",
           527 => x"88170827",
           528 => x"80e53875",
           529 => x"33557482",
           530 => x"2e893874",
           531 => x"832eae38",
           532 => x"80d53974",
           533 => x"54761083",
           534 => x"fe065376",
           535 => x"882a8c17",
           536 => x"08055288",
           537 => x"3d705255",
           538 => x"fdbd3f88",
           539 => x"08b93874",
           540 => x"51fef83f",
           541 => x"880883ff",
           542 => x"ff0655ad",
           543 => x"39845476",
           544 => x"822b83fc",
           545 => x"06537687",
           546 => x"2a8c1708",
           547 => x"0552883d",
           548 => x"705255fd",
           549 => x"923f8808",
           550 => x"8e387451",
           551 => x"fee23f88",
           552 => x"08f00a06",
           553 => x"55833981",
           554 => x"5574880c",
           555 => x"893d0d04",
           556 => x"fb3d0d0b",
           557 => x"0b0b9ff4",
           558 => x"08fe1988",
           559 => x"1208fe05",
           560 => x"55565480",
           561 => x"56747327",
           562 => x"8d388214",
           563 => x"33757129",
           564 => x"94160805",
           565 => x"57537588",
           566 => x"0c873d0d",
           567 => x"04fd3d0d",
           568 => x"7554800b",
           569 => x"0b0b0b9f",
           570 => x"f4087033",
           571 => x"51535371",
           572 => x"832e0981",
           573 => x"068c3894",
           574 => x"1451fdef",
           575 => x"3f880890",
           576 => x"2b539a14",
           577 => x"51fde43f",
           578 => x"880883ff",
           579 => x"ff067307",
           580 => x"880c853d",
           581 => x"0d04fc3d",
           582 => x"0d760b0b",
           583 => x"0b9ff408",
           584 => x"55558075",
           585 => x"23881508",
           586 => x"5372812e",
           587 => x"88388814",
           588 => x"08732685",
           589 => x"388152b0",
           590 => x"39729038",
           591 => x"73335271",
           592 => x"832e0981",
           593 => x"06853890",
           594 => x"14085372",
           595 => x"8c160c72",
           596 => x"802e8b38",
           597 => x"7251fed8",
           598 => x"3f880852",
           599 => x"85399014",
           600 => x"08527190",
           601 => x"160c8052",
           602 => x"71880c86",
           603 => x"3d0d04fa",
           604 => x"3d0d780b",
           605 => x"0b0b9ff4",
           606 => x"08712281",
           607 => x"057083ff",
           608 => x"ff065754",
           609 => x"57557380",
           610 => x"2e883890",
           611 => x"15085372",
           612 => x"86388352",
           613 => x"80dc3973",
           614 => x"8f065271",
           615 => x"80cf3881",
           616 => x"1390160c",
           617 => x"8c150853",
           618 => x"728f3883",
           619 => x"0b841722",
           620 => x"57527376",
           621 => x"27bc38b5",
           622 => x"39821633",
           623 => x"ff057484",
           624 => x"2a065271",
           625 => x"a8387251",
           626 => x"fcdf3f81",
           627 => x"52718808",
           628 => x"27a03883",
           629 => x"52880888",
           630 => x"17082796",
           631 => x"3888088c",
           632 => x"160c8808",
           633 => x"51fdc93f",
           634 => x"88089016",
           635 => x"0c737523",
           636 => x"80527188",
           637 => x"0c883d0d",
           638 => x"04f23d0d",
           639 => x"60626458",
           640 => x"5e5c7533",
           641 => x"5574a02e",
           642 => x"09810688",
           643 => x"38811670",
           644 => x"4456ef39",
           645 => x"62703356",
           646 => x"5674af2e",
           647 => x"09810684",
           648 => x"38811643",
           649 => x"800b881d",
           650 => x"0c627033",
           651 => x"5155749f",
           652 => x"268f387b",
           653 => x"51fddf3f",
           654 => x"88085680",
           655 => x"7d3482d3",
           656 => x"39933d84",
           657 => x"1d087058",
           658 => x"5a5f8a55",
           659 => x"a0767081",
           660 => x"055834ff",
           661 => x"155574ff",
           662 => x"2e098106",
           663 => x"ef388070",
           664 => x"595b887f",
           665 => x"085f5a7a",
           666 => x"811c7081",
           667 => x"ff066013",
           668 => x"703370af",
           669 => x"327030a0",
           670 => x"73277180",
           671 => x"25075151",
           672 => x"525b535d",
           673 => x"57557480",
           674 => x"c73876ae",
           675 => x"2e098106",
           676 => x"83388155",
           677 => x"777a2775",
           678 => x"07557480",
           679 => x"2e9f3879",
           680 => x"88327030",
           681 => x"78ae3270",
           682 => x"30707307",
           683 => x"9f2a5351",
           684 => x"57515675",
           685 => x"9b388858",
           686 => x"8b5affab",
           687 => x"39778119",
           688 => x"7081ff06",
           689 => x"721c535a",
           690 => x"57557675",
           691 => x"34ff9839",
           692 => x"7a1e7f0c",
           693 => x"805576a0",
           694 => x"26833881",
           695 => x"55748b1a",
           696 => x"347b51fc",
           697 => x"b13f8808",
           698 => x"80ef38a0",
           699 => x"547b2270",
           700 => x"852b83e0",
           701 => x"06545590",
           702 => x"1c08527c",
           703 => x"51f8a83f",
           704 => x"88085788",
           705 => x"0880fb38",
           706 => x"7c335574",
           707 => x"802e80ee",
           708 => x"388b1d33",
           709 => x"70832a70",
           710 => x"81065156",
           711 => x"5674b238",
           712 => x"8b7d841e",
           713 => x"08880859",
           714 => x"5b5b58ff",
           715 => x"185877ff",
           716 => x"2e9a3879",
           717 => x"7081055b",
           718 => x"33797081",
           719 => x"055b3371",
           720 => x"71315256",
           721 => x"5675802e",
           722 => x"e2388639",
           723 => x"75802e92",
           724 => x"387b51fc",
           725 => x"9a3fff8e",
           726 => x"39880856",
           727 => x"8808b438",
           728 => x"83397656",
           729 => x"841c088b",
           730 => x"11335155",
           731 => x"74a5388b",
           732 => x"1d337084",
           733 => x"2a708106",
           734 => x"51565674",
           735 => x"89388356",
           736 => x"92398156",
           737 => x"8e397c51",
           738 => x"fad33f88",
           739 => x"08881d0c",
           740 => x"fdaf3975",
           741 => x"880c903d",
           742 => x"0d04f93d",
           743 => x"0d797b59",
           744 => x"57825483",
           745 => x"fe537752",
           746 => x"7651f6fb",
           747 => x"3f835688",
           748 => x"0880e738",
           749 => x"7651f8b3",
           750 => x"3f880883",
           751 => x"ffff0655",
           752 => x"82567482",
           753 => x"d4d52e09",
           754 => x"810680ce",
           755 => x"387554b6",
           756 => x"53775276",
           757 => x"51f6d03f",
           758 => x"88085688",
           759 => x"08943876",
           760 => x"51f8883f",
           761 => x"880883ff",
           762 => x"ff065574",
           763 => x"8182c62e",
           764 => x"a9388254",
           765 => x"80d25377",
           766 => x"527651f6",
           767 => x"aa3f8808",
           768 => x"56880894",
           769 => x"387651f7",
           770 => x"e23f8808",
           771 => x"83ffff06",
           772 => x"55748182",
           773 => x"c62e8338",
           774 => x"81567588",
           775 => x"0c893d0d",
           776 => x"04ed3d0d",
           777 => x"6559800b",
           778 => x"0b0b0b9f",
           779 => x"f40cf59b",
           780 => x"3f880881",
           781 => x"06558256",
           782 => x"7482f238",
           783 => x"7475538d",
           784 => x"3d705357",
           785 => x"5afed33f",
           786 => x"880881ff",
           787 => x"06577681",
           788 => x"2e098106",
           789 => x"b3389054",
           790 => x"83be5374",
           791 => x"527551f5",
           792 => x"c63f8808",
           793 => x"ab388d3d",
           794 => x"33557480",
           795 => x"2eac3895",
           796 => x"3de40551",
           797 => x"f78a3f88",
           798 => x"08880853",
           799 => x"76525afe",
           800 => x"993f8808",
           801 => x"81ff0657",
           802 => x"76832e09",
           803 => x"81068638",
           804 => x"81568299",
           805 => x"3976802e",
           806 => x"86388656",
           807 => x"828f39a4",
           808 => x"548d5379",
           809 => x"527551f4",
           810 => x"fe3f8156",
           811 => x"880881fd",
           812 => x"38953de5",
           813 => x"0551f6b3",
           814 => x"3f880883",
           815 => x"ffff0658",
           816 => x"778c3895",
           817 => x"3df30551",
           818 => x"f6b63f88",
           819 => x"085802af",
           820 => x"05337871",
           821 => x"29028805",
           822 => x"ad057054",
           823 => x"52595bf6",
           824 => x"8a3f8808",
           825 => x"83ffff06",
           826 => x"7a058c1a",
           827 => x"0c8c3d33",
           828 => x"821a3495",
           829 => x"3de00551",
           830 => x"f5f13f88",
           831 => x"08841a23",
           832 => x"953de205",
           833 => x"51f5e43f",
           834 => x"880883ff",
           835 => x"ff065675",
           836 => x"8c38953d",
           837 => x"ef0551f5",
           838 => x"e73f8808",
           839 => x"567a51f5",
           840 => x"ca3f8808",
           841 => x"83ffff06",
           842 => x"76713179",
           843 => x"31841b22",
           844 => x"70842a82",
           845 => x"1d335672",
           846 => x"71315559",
           847 => x"5c5155ee",
           848 => x"c43f8808",
           849 => x"82057088",
           850 => x"1b0c8808",
           851 => x"e08a0556",
           852 => x"567483df",
           853 => x"fe268338",
           854 => x"825783ff",
           855 => x"f6762785",
           856 => x"38835789",
           857 => x"39865676",
           858 => x"802e80c1",
           859 => x"38767934",
           860 => x"76832e09",
           861 => x"81069038",
           862 => x"953dfb05",
           863 => x"51f5813f",
           864 => x"8808901a",
           865 => x"0c88398c",
           866 => x"19081890",
           867 => x"1a0c7983",
           868 => x"ffff068c",
           869 => x"1a081971",
           870 => x"842a0594",
           871 => x"1b0c5580",
           872 => x"0b811a34",
           873 => x"780b0b0b",
           874 => x"9ff40c80",
           875 => x"5675880c",
           876 => x"953d0d04",
           877 => x"ea3d0d0b",
           878 => x"0b0b9ff4",
           879 => x"08558554",
           880 => x"74802e80",
           881 => x"df38800b",
           882 => x"81163498",
           883 => x"3de01145",
           884 => x"6954893d",
           885 => x"705457ec",
           886 => x"0551f89d",
           887 => x"3f880854",
           888 => x"880880c0",
           889 => x"38883d33",
           890 => x"5473802e",
           891 => x"933802a7",
           892 => x"05337084",
           893 => x"2a708106",
           894 => x"51555773",
           895 => x"802e8538",
           896 => x"8354a139",
           897 => x"7551f5d5",
           898 => x"3f8808a0",
           899 => x"160c983d",
           900 => x"dc0551f3",
           901 => x"eb3f8808",
           902 => x"9c160c73",
           903 => x"98160c81",
           904 => x"0b811634",
           905 => x"73880c98",
           906 => x"3d0d04f6",
           907 => x"3d0d7d7f",
           908 => x"7e0b0b0b",
           909 => x"9ff40859",
           910 => x"5b5c5880",
           911 => x"7b0c8557",
           912 => x"75802e81",
           913 => x"d1388116",
           914 => x"33810655",
           915 => x"84577480",
           916 => x"2e81c338",
           917 => x"91397481",
           918 => x"17348639",
           919 => x"800b8117",
           920 => x"34815781",
           921 => x"b1399c16",
           922 => x"08981708",
           923 => x"31557478",
           924 => x"27833874",
           925 => x"5877802e",
           926 => x"819a3898",
           927 => x"16087083",
           928 => x"ff065657",
           929 => x"7480c738",
           930 => x"821633ff",
           931 => x"0577892a",
           932 => x"067081ff",
           933 => x"065b5579",
           934 => x"9e387687",
           935 => x"38a01608",
           936 => x"558b39a4",
           937 => x"160851f3",
           938 => x"803f8808",
           939 => x"55817527",
           940 => x"ffaa3874",
           941 => x"a4170ca4",
           942 => x"160851f3",
           943 => x"f33f8808",
           944 => x"55880880",
           945 => x"2eff8f38",
           946 => x"88081aa8",
           947 => x"170c9816",
           948 => x"0883ff06",
           949 => x"84807131",
           950 => x"51557775",
           951 => x"27833877",
           952 => x"55745498",
           953 => x"160883ff",
           954 => x"0653a816",
           955 => x"08527851",
           956 => x"f0b53f88",
           957 => x"08fee538",
           958 => x"98160815",
           959 => x"98170c77",
           960 => x"75317b08",
           961 => x"167c0c58",
           962 => x"78802efe",
           963 => x"e8387419",
           964 => x"59fee239",
           965 => x"80577688",
           966 => x"0c8c3d0d",
           967 => x"04fb3d0d",
           968 => x"87c0948c",
           969 => x"08548784",
           970 => x"80527351",
           971 => x"ead73f88",
           972 => x"08902b87",
           973 => x"c0948c08",
           974 => x"56548784",
           975 => x"80527451",
           976 => x"eac33f73",
           977 => x"88080787",
           978 => x"c0948c0c",
           979 => x"87c0949c",
           980 => x"08548784",
           981 => x"80527351",
           982 => x"eaab3f88",
           983 => x"08902b87",
           984 => x"c0949c08",
           985 => x"56548784",
           986 => x"80527451",
           987 => x"ea973f73",
           988 => x"88080787",
           989 => x"c0949c0c",
           990 => x"8c80830b",
           991 => x"87c09484",
           992 => x"0c8c8083",
           993 => x"0b87c094",
           994 => x"940c9ff8",
           995 => x"51f9923f",
           996 => x"8808b838",
           997 => x"9fe051fc",
           998 => x"9b3f8808",
           999 => x"ae38a080",
          1000 => x"0b880887",
          1001 => x"c098880c",
          1002 => x"55873dfc",
          1003 => x"05538480",
          1004 => x"527451fc",
          1005 => x"f63f8808",
          1006 => x"8d387554",
          1007 => x"73802e86",
          1008 => x"38731555",
          1009 => x"e439a080",
          1010 => x"54730480",
          1011 => x"54fb3900",
          1012 => x"00ffffff",
          1013 => x"ff00ffff",
          1014 => x"ffff00ff",
          1015 => x"ffffff00",
          1016 => x"424f4f54",
          1017 => x"54494e59",
          1018 => x"2e524f4d",
          1019 => x"00000000",
          1020 => x"01000000",
        others => x"00000000"
    );

begin

process (clk)
begin
    if (clk'event and clk = '1') then
        if (memAWriteEnable = '1') and (memBWriteEnable = '1') and (memAAddr=memBAddr) and (memAWrite/=memBWrite) then
            report "write collision" severity failure;
        end if;
    
        if (memAWriteEnable = '1') then
            ram(to_integer(unsigned(memAAddr(ADDR_32BIT_BRAM_RANGE)))) := memAWrite;
            memARead <= memAWrite;
        else
            memARead <= ram(to_integer(unsigned(memAAddr(ADDR_32BIT_BRAM_RANGE))));
        end if;
    end if;
end process;

process (clk)
begin
    if (clk'event and clk = '1') then
        if (memBWriteEnable = '1') then
            ram(to_integer(unsigned(memBAddr(ADDR_32BIT_BRAM_RANGE)))) := memBWrite;
            memBRead <= memBWrite;
        else
            memBRead <= ram(to_integer(unsigned(memBAddr(ADDR_32BIT_BRAM_RANGE))));
        end if;
    end if;
end process;


end arch;

