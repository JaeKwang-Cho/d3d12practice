//
// Generated by Microsoft (R) HLSL Shader Compiler 10.1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_POSITION              0   xyzw        0      POS   float       
// COLOR                    0   xyzw        1     NONE   float   xyzw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Target                0   xyzw        0   TARGET   float   xyzw
//
ps_5_1
dcl_globalFlags refactoringAllowed | skipOptimization
dcl_input_ps linear v1.xyzw
dcl_output o0.xyzw
//
// Initial variable locations:
//   v0.x <- _pin.PosH.x; v0.y <- _pin.PosH.y; v0.z <- _pin.PosH.z; v0.w <- _pin.PosH.w; 
//   v1.x <- _pin.Color.x; v1.y <- _pin.Color.y; v1.z <- _pin.Color.z; v1.w <- _pin.Color.w; 
//   o0.x <- <PS return value>.x; o0.y <- <PS return value>.y; o0.z <- <PS return value>.z; o0.w <- <PS return value>.w
//
#line 39 "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_Shader.hlsl"
mov o0.xyzw, v1.xyzw
ret 
// Approximately 2 instruction slots used
