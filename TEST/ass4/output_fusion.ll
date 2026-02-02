; ModuleID = 'test_fusion.ll'
source_filename = "test_fusion.c"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx15.0.0"

; Function Attrs: nofree norecurse nosync nounwind ssp memory(argmem: readwrite) uwtable
define void @f(ptr nocapture noundef %0, ptr nocapture noundef %1, i32 noundef %2) local_unnamed_addr #0 {
  %4 = icmp sgt i32 %2, 0
  br i1 %4, label %5, label %7

5:                                                ; preds = %3
  %6 = zext i32 %2 to i64
  br label %9

.loopexit1:                                       ; preds = %9
  br label %.loopexit

7:                                                ; preds = %3
  %8 = icmp sgt i32 %2, 0
  br i1 %8, label %.loopexit, label %19

9:                                                ; preds = %9, %5
  %10 = phi i64 [ 0, %5 ], [ %14, %9 ]
  %11 = getelementptr inbounds i32, ptr %0, i64 %10
  %12 = load i32, ptr %11, align 4, !tbaa !5
  %13 = add nsw i32 %12, 1
  store i32 %13, ptr %11, align 4, !tbaa !5
  %14 = add nuw nsw i64 %10, 1
  %15 = getelementptr inbounds i32, ptr %1, i64 %10
  %16 = load i32, ptr %15, align 4, !tbaa !5
  %17 = add nsw i32 %16, 2
  store i32 %17, ptr %15, align 4, !tbaa !5
  %18 = icmp eq i64 %14, %6
  br i1 %18, label %.loopexit1, label %9, !llvm.loop !9

.loopexit:                                        ; preds = %7, %.loopexit1
  br label %19

19:                                               ; preds = %.loopexit, %7
  ret void
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
