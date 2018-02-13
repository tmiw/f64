`timescale 1ns/1ps
module memory_controller(
	clk,
	address_in,
	data_in,
	data_out,
	read_en,
	write_en,
	sram_addr,
	sram_data,
	sram_ce_inv,
	sram_oe_inv,
	sram_we_inv,
	video_ram_addr,
	video_ram_data,
	video_ram_we
);

input clk;
input [15:0] address_in;
input [15:0] data_in;
output reg [15:0] data_out;
input write_en;
input read_en;

output reg [20:0] sram_addr;
inout [7:0] sram_data;
output reg sram_ce_inv;
output reg sram_oe_inv;
output reg sram_we_inv;

output reg [11:0] video_ram_addr;
output reg [15:0] video_ram_data = 0;
output reg video_ram_we = 0;

reg [7:0] sram_data_out;
assign sram_data = !sram_oe_inv ? 8'bz : sram_data_out;

reg current_byte;

always @(posedge clk)
begin
	if (read_en)
	begin
		if (address_in >= 16'hC000)
		begin
			// I/O area: 0xC000-0xFFFF
			data_out <= 0;
		end
		else if (address_in <= 16'h010b)
		begin
			// Simple program ROM for testing
			// Prints "Hello" at (0,0) and stops.
			case (address_in)
			16'h0000: data_out <= 16'hf82f; // const: 0xf82f
			16'h0001: data_out <= 16'h0748; // const: 0x0748 (H)
			16'h0002: data_out <= 16'h0765; // const: 0x0765 (e)
			16'h0003: data_out <= 16'h076c; // const: 0x076c (l)
			16'h0004: data_out <= 16'h076c; // const: 0x076c (l)
			16'h0005: data_out <= 16'h076f; // const: 0x076f (o)
			16'h0100: data_out <= 16'h4400; // ld r1, r0, 0
			16'h0101: data_out <= 16'h4801; // ld r2, r0, 1
			16'h0102: data_out <= 16'h6500; // st r1, r2, 0
			16'h0103: data_out <= 16'h4802; // ld r2, r0, 2
			16'h0104: data_out <= 16'h6501; // st r1, r2, 1
			16'h0105: data_out <= 16'h4803; // ld r2, r0, 3
			16'h0106: data_out <= 16'h6502; // st r1, r2, 2
			16'h0107: data_out <= 16'h4804; // ld r2, r0, 4
			16'h0108: data_out <= 16'h6503; // st r1, r2, 3
			16'h0109: data_out <= 16'h4805; // ld r2, r0, 5
			16'h010a: data_out <= 16'h6504; // st r1, r2, 4
			16'h010b: data_out <= 16'h8ff6; // br 0x0102
			default: data_out <= 0;
			endcase
		end
		else
		begin
			sram_addr <= {4'b0, address_in, current_byte};
			sram_ce_inv <= 0;
			sram_oe_inv <= 0;
			sram_we_inv <= 1;
			if (current_byte)
				data_out[7:0] <= sram_data;
			else
				data_out[15:8] <= sram_data;
			current_byte <= current_byte + 1'b1;
		end
	end
	else if (write_en)
	begin
		if (address_in >= 16'hC000)
		begin
			// I/O area: 0xC000-0xFFFF
			if (address_in >= 16'hF82F)
			begin
				// Video RAM
				video_ram_addr <= address_in - 16'hF82F;
				video_ram_data <= data_in;
				video_ram_we <= 1;
			end
			else
			begin
				video_ram_addr <= 0;
				video_ram_data <= 0;
				video_ram_we <= 0;
			end
		end
		else
		begin
			sram_addr <= {4'b0, address_in, current_byte};
			sram_ce_inv <= 0;
			sram_oe_inv <= 1;
			sram_we_inv <= 0;
			video_ram_addr <= 0;
			video_ram_data <= 0;
			video_ram_we <= 0;
			if (current_byte)
				sram_data_out <= data_in[15:8];
			else
				sram_data_out <= data_in[7:0];
			current_byte <= current_byte + 1'b1;
		end
	end
	else 
	begin
		current_byte <= 0;
		sram_addr <= 0;
		sram_ce_inv <= 1;
		sram_oe_inv <= 1;
		sram_we_inv <= 1;
	end
end

endmodule