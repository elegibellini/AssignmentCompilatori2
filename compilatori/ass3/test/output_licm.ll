; ModuleID = 'test_licm.ll'
source_filename = "test_licm.c"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx15.0.0"

@.str = private unnamed_addr constant [14 x i8] c"risultato=%d\0A\00", align 1

; Function Attrs: noinline nounwind ssp uwtable
define i32 @mylicm(i32 noundef %0) #0 {
  %2 = add nsw i32 3, 2
  br label %3

3:                                                ; preds = %7, %1
  %.01 = phi i32 [ 0, %1 ], [ %6, %7 ]
  %.0 = phi i32 [ 0, %1 ], [ %8, %7 ]
  %4 = icmp slt i32 %.0, %0
  br i1 %4, label %5, label %9

5:                                                ; preds = %3
  %6 = add nsw i32 %.01, %2
  br label %7

7:                                                ; preds = %5
  %8 = add nsw i32 %.0, 1
  br label %3, !llvm.loop !6

9:                                                ; preds = %3
  %.01.lcssa = phi i32 [ %.01, %3 ]
  ret i32 %.01.lcssa
}

; Function Attrs: noinline nounwind ssp uwtable
define i32 @main() #0 {
  %1 = call i32 @mylicm(i32 noundef 5)
  %2 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %1)
  ret i32 0
}

declare i32 @printf(ptr noundef, ...) #1

attributes #0 = { noinline nounwind ssp uwtable "darwin-stkchk-strong-link" "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "probe-stack"="___chkstk_darwin" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cmov,+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "tune-cpu"="generic" }
attributes #1 = { "darwin-stkchk-strong-link" "frame-pointer"="all" "no-trapping-math"="true" "probe-stack"="___chkstk_darwin" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cmov,+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 2, !"SDK Version", [2 x i32] [i32 26, i32 2]}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 8, !"PIC Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Apple clang version 17.0.0 (clang-1700.6.3.2)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
