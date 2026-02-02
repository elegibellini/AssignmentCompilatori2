; ModuleID = 'test_fusion.c'
source_filename = "test_fusion.c"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx15.0.0"

; Function Attrs: nofree norecurse nosync nounwind ssp memory(argmem: readwrite) uwtable
define void @f(ptr nocapture noundef %0, ptr nocapture noundef %1, i32 noundef %2) local_unnamed_addr #0 {
  %4 = icmp sgt i32 %2, 0
  br i1 %4, label %5, label %7

5:                                                ; preds = %3
  %6 = zext i32 %2 to i64
  br label %11

7:                                                ; preds = %11, %3
  %8 = icmp sgt i32 %2, 0
  br i1 %8, label %9, label %18

9:                                                ; preds = %7
  %10 = zext i32 %2 to i64
  br label %19

11:                                               ; preds = %5, %11
  %12 = phi i64 [ 0, %5 ], [ %16, %11 ]
  %13 = getelementptr inbounds i32, ptr %0, i64 %12
  %14 = load i32, ptr %13, align 4, !tbaa !5
  %15 = add nsw i32 %14, 1
  store i32 %15, ptr %13, align 4, !tbaa !5
  %16 = add nuw nsw i64 %12, 1
  %17 = icmp eq i64 %16, %6
  br i1 %17, label %7, label %11, !llvm.loop !9

18:                                               ; preds = %19, %7
  ret void

19:                                               ; preds = %9, %19
  %20 = phi i64 [ 0, %9 ], [ %24, %19 ]
  %21 = getelementptr inbounds i32, ptr %1, i64 %20
  %22 = load i32, ptr %21, align 4, !tbaa !5
  %23 = add nsw i32 %22, 2
  store i32 %23, ptr %21, align 4, !tbaa !5
  %24 = add nuw nsw i64 %20, 1
  %25 = icmp eq i64 %24, %10
  br i1 %25, label %18, label %19, !llvm.loop !12
}

attributes #0 = { nofree norecurse nosync nounwind ssp memory(argmem: readwrite) uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cmov,+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"uwtable", i32 2}
!3 = !{i32 7, !"frame-pointer", i32 2}
!4 = !{!"clang version 17.0.6"}
!5 = !{!6, !6, i64 0}
!6 = !{!"int", !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C/C++ TBAA"}
!9 = distinct !{!9, !10, !11}
!10 = !{!"llvm.loop.mustprogress"}
!11 = !{!"llvm.loop.unroll.disable"}
!12 = distinct !{!12, !10, !11}
