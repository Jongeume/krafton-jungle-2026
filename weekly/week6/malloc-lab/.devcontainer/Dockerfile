# malloc-lab 실습 환경 (CS:APP Malloc Lab)
# Makefile 이 -m32 (32비트 x86) 로 빌드하므로 gcc-multilib 가 필요하다.
# x86-64 리눅스 전용 — M1 맥(ARM)에서는 -m32 빌드가 되지 않는다.
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential gcc-multilib gdb make ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# gdb 시작 시 debuginfod(디버그 심볼 자동 다운로드) 질문 끄기
RUN echo 'set debuginfod enabled off' >> /root/.gdbinit

WORKDIR /work
CMD ["/bin/bash"]
