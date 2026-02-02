; ModuleID = 'Fibonacci.c'
source_filename = "Fibonacci.c"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx15.0.0"

%struct.__va_list_tag = type { i32, i32, ptr, ptr }

@__stdoutp = external local_unnamed_addr global ptr, align 8
@.str = private unnamed_addr constant [9 x i8] c"f(0) = 0\00", align 1
@.str.1 = private unnamed_addr constant [9 x i8] c"f(1) = 1\00", align 1
@.str.2 = private unnamed_addr constant [22 x i8] c"f(%d) = f(%d) + f(%d)\00", align 1

; Function Attrs: nofree nounwind ssp uwtable
define noundef i32 @printf(ptr nocapture noundef readonly %0, ...) local_unnamed_addr #0 {
  %2 = alloca [1 x %struct.__va_list_tag], align 16
  call void @llvm.lifetime.start.p0(i64 24, ptr nonnull %2) #4
  call void @llvm.va_start.p0(ptr nonnull %2)
  %3 = load ptr, ptr @__stdoutp, align 8, !tbaa !6
  %4 = call i32 @vfprintf(ptr noundef %3, ptr noundef %0, ptr noundef nonnull %2) #4
  call void @llvm.va_end.p0(ptr nonnull %2)
  call void @llvm.lifetime.end.p0(i64 24, ptr nonnull %2) #4
  ret i32 %4
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #1

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn
declare void @llvm.va_start.p0(ptr) #2

; Function Attrs: nofree nounwind
declare noundef i32 @vfprintf(ptr nocapture noundef, ptr nocapture noundef readonly, ptr noundef) local_unnamed_addr #3

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn
declare void @llvm.va_end.p0(ptr) #2

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #1

; Function Attrs: nofree nounwind ssp uwtable
define i32 @Fibonacci(i32 noundef %0) local_unnamed_addr #0 {
  br label %2

2:                                                ; preds = %5, %1
  %3 = phi i32 [ 0, %1 ], [ %10, %5 ]
  %4 = phi i32 [ %0, %1 ], [ %7, %5 ]
  switch i32 %4, label %5 [
    i32 0, label %12
    i32 1, label %11
  ]

5:                                                ; preds = %2
  %6 = add nsw i32 %4, -1
  %7 = add nsw i32 %4, -2
  %8 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.2, i32 noundef %4, i32 noundef %6, i32 noundef %7)
  %9 = tail call i32 @Fibonacci(i32 noundef %6)
  %10 = add nsw i32 %9, %3
  br label %2

11:                                               ; preds = %2
  br label %12

12:                                               ; preds = %2, %11
  %13 = phi ptr [ @.str.1, %11 ], [ @.str, %2 ]
  %14 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) %13)
  %15 = add nsw i32 %4, %3
  ret i32 %15
}

attributes #0 = { nofree nounwind ssp uwtable "darwin-stkchk-strong-link" "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "probe-stack"="___chkstk_darwin" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cmov,+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "tune-cpu"="generic" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { mustprogress nocallback nofree nosync nounwind willreturn }
attributes #3 = { nofree nounwind "darwin-stkchk-strong-link" "frame-pointer"="all" "no-trapping-math"="true" "probe-stack"="___chkstk_darwin" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cmov,+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "tune-cpu"="generic" }
attributes #4 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 2, !"SDK Version", [2 x i32] [i32 26, i32 2]}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 8, !"PIC Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Apple clang version 17.0.0 (clang-1700.6.3.2)"}
!6 = !{!7, !7, i64 0}
!7 = !{!"any pointer", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C/C++ TBAA"}
