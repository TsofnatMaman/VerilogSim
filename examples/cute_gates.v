module cute_gates (
    input wire a,
    input wire b,
    input wire c,
    input wire d,
    output wire ab_and,
    output wire cd_or,
    output wire not_d,
    output wire y_xor,
    output wire y_mix
);
    assign ab_and = a & b;
    assign cd_or = c | d;
    assign y_xor = a ^ c;
    assign not_d = ~d;
    assign y_mix = ab_and ^ cd_or;
endmodule
