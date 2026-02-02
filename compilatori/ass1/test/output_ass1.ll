; ModuleID = 'prova.ll'
source_filename = "prova.ll"

define void @example_function(ptr %a, ptr %b, ptr %c, ptr %d, ptr %e, ptr %f, ptr %g, ptr %h, ptr %i, ptr %l, ptr %m, ptr %n, ptr %o) {
entry:
  %0 = load i32, ptr %a, align 4
  %1 = add i32 %0, 0
  store i32 %0, ptr %a, align 4
  %2 = load i32, ptr %b, align 4
  %3 = mul i32 1, %2
  store i32 %2, ptr %b, align 4
  %4 = load i32, ptr %c, align 4
  %5 = mul i32 %4, 8
  %6 = shl i32 %4, 3
  store i32 %6, ptr %c, align 4
  %7 = load i32, ptr %d, align 4
  %8 = sdiv i32 %7, 16
  %9 = ashr i32 %7, 4
  store i32 %9, ptr %d, align 4
  %10 = load i32, ptr %e, align 4
  %11 = sub i32 %10, 0
  store i32 %10, ptr %e, align 4
  %12 = load i32, ptr %g, align 4
  %13 = add i32 %12, 3
  store i32 %13, ptr %f, align 4
  %14 = sub i32 %13, 3
  store i32 %12, ptr %h, align 4
  %15 = load i32, ptr %l, align 4
  %16 = sub i32 %15, 1
  store i32 %16, ptr %i, align 4
  %17 = add i32 %16, 1
  store i32 %15, ptr %m, align 4
  store i32 3, ptr %n, align 4
  %18 = load i32, ptr %n, align 4
  %19 = mul i32 %18, 15
  %20 = shl i32 %18, 4
  %21 = sub i32 %20, %18
  store i32 %21, ptr %n, align 4
  store i32 10, ptr %o, align 4
  %22 = load i32, ptr %o, align 4
  %23 = mul i32 18, %22
  store i32 %23, ptr %o, align 4
  ret void
}
