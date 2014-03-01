; ModuleID = 'testit.cc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.8.0"

%class.B = type { %class.A }
%class.A = type { i32 (...)**, i32 }

@_ZTV1A = linkonce_odr unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8* }* @_ZTI1A to i8*), i8* bitcast (i32 (%class.A*, i32)* @_ZN1A3addEi to i8*)]
@_ZTVN10__cxxabiv117__class_type_infoE = external global i8*
@_ZTS1A = linkonce_odr constant [3 x i8] c"1A\00"
@_ZTI1A = linkonce_odr unnamed_addr constant { i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8** @_ZTVN10__cxxabiv117__class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([3 x i8]* @_ZTS1A, i32 0, i32 0) }
@_ZTV1B = linkonce_odr unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8*, i8* }* @_ZTI1B to i8*), i8* bitcast (i32 (%class.B*, i32)* @_ZN1B3addEi to i8*)]
@_ZTVN10__cxxabiv120__si_class_type_infoE = external global i8*
@_ZTS1B = linkonce_odr constant [3 x i8] c"1B\00"
@_ZTI1B = linkonce_odr unnamed_addr constant { i8*, i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8** @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([3 x i8]* @_ZTS1B, i32 0, i32 0), i8* bitcast ({ i8*, i8* }* @_ZTI1A to i8*) }

; Function Attrs: ssp uwtable
define i32 @main(i32 %argc, i8** %argv) #0 {
entry:
  %retval = alloca i32, align 4
  %argc.addr = alloca i32, align 4
  %argv.addr = alloca i8**, align 8
  %b = alloca %class.B, align 8
  %a = alloca %class.A, align 8
  store i32 0, i32* %retval
  store i32 %argc, i32* %argc.addr, align 4
  store i8** %argv, i8*** %argv.addr, align 8
  call void @_ZN1BC1Ei(%class.B* %b, i32 10)
  call void @_ZN1AC1Ei(%class.A* %a, i32 10)
  %call = call i32 @_ZN1B3addEi(%class.B* %b, i32 10)
  ret i32 0
}

; Function Attrs: ssp uwtable
define linkonce_odr void @_ZN1BC1Ei(%class.B* %this, i32 %i) unnamed_addr #0 align 2 {
entry:
  %this.addr = alloca %class.B*, align 8
  %i.addr = alloca i32, align 4
  store %class.B* %this, %class.B** %this.addr, align 8
  store i32 %i, i32* %i.addr, align 4
  %this1 = load %class.B** %this.addr
  %0 = load i32* %i.addr, align 4
  call void @_ZN1BC2Ei(%class.B* %this1, i32 %0)
  ret void
}

; Function Attrs: ssp uwtable
define linkonce_odr void @_ZN1AC1Ei(%class.A* %this, i32 %i) unnamed_addr #0 align 2 {
entry:
  %this.addr = alloca %class.A*, align 8
  %i.addr = alloca i32, align 4
  store %class.A* %this, %class.A** %this.addr, align 8
  store i32 %i, i32* %i.addr, align 4
  %this1 = load %class.A** %this.addr
  %0 = load i32* %i.addr, align 4
  call void @_ZN1AC2Ei(%class.A* %this1, i32 %0)
  ret void
}

; Function Attrs: nounwind ssp uwtable
define linkonce_odr i32 @_ZN1B3addEi(%class.B* %this, i32 %y) unnamed_addr #1 align 2 {
entry:
  %this.addr = alloca %class.B*, align 8
  %y.addr = alloca i32, align 4
  store %class.B* %this, %class.B** %this.addr, align 8
  store i32 %y, i32* %y.addr, align 4
  %this1 = load %class.B** %this.addr
  %0 = bitcast %class.B* %this1 to %class.A*
  %x = getelementptr inbounds %class.A* %0, i32 0, i32 1
  %1 = load i32* %x, align 4
  %2 = load i32* %y.addr, align 4
  %add = add nsw i32 %1, %2
  ret i32 %add
}

; Function Attrs: nounwind ssp uwtable
define linkonce_odr void @_ZN1AC2Ei(%class.A* %this, i32 %i) unnamed_addr #1 align 2 {
entry:
  %this.addr = alloca %class.A*, align 8
  %i.addr = alloca i32, align 4
  store %class.A* %this, %class.A** %this.addr, align 8
  store i32 %i, i32* %i.addr, align 4
  %this1 = load %class.A** %this.addr
  %0 = bitcast %class.A* %this1 to i8***
  store i8** getelementptr inbounds ([3 x i8*]* @_ZTV1A, i64 0, i64 2), i8*** %0
  %x = getelementptr inbounds %class.A* %this1, i32 0, i32 1
  %1 = load i32* %i.addr, align 4
  store i32 %1, i32* %x, align 4
  ret void
}

; Function Attrs: nounwind ssp uwtable
define linkonce_odr i32 @_ZN1A3addEi(%class.A* %this, i32 %y) unnamed_addr #1 align 2 {
entry:
  %this.addr = alloca %class.A*, align 8
  %y.addr = alloca i32, align 4
  store %class.A* %this, %class.A** %this.addr, align 8
  store i32 %y, i32* %y.addr, align 4
  %this1 = load %class.A** %this.addr
  %x = getelementptr inbounds %class.A* %this1, i32 0, i32 1
  %0 = load i32* %x, align 4
  %1 = load i32* %y.addr, align 4
  %add = add nsw i32 %0, %1
  ret i32 %add
}

; Function Attrs: nounwind ssp uwtable
define linkonce_odr void @_ZN1BC2Ei(%class.B* %this, i32 %i) unnamed_addr #1 align 2 {
entry:
  %this.addr = alloca %class.B*, align 8
  %i.addr = alloca i32, align 4
  store %class.B* %this, %class.B** %this.addr, align 8
  store i32 %i, i32* %i.addr, align 4
  %this1 = load %class.B** %this.addr
  %0 = bitcast %class.B* %this1 to %class.A*
  %1 = load i32* %i.addr, align 4
  call void @_ZN1AC2Ei(%class.A* %0, i32 %1)
  %2 = bitcast %class.B* %this1 to i8***
  store i8** getelementptr inbounds ([3 x i8*]* @_ZTV1B, i64 0, i64 2), i8*** %2
  ret void
}

attributes #0 = { ssp uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind ssp uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"clang version 3.4 (tags/RELEASE_34/final)"}
